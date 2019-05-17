/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2019, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <asm/ioctls.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph Coffland");
MODULE_DESCRIPTION("Buildbotics controller serial port driver");
MODULE_VERSION("0.2");


#define DEVICE_NAME "ttyBB0"
#define BUF_SIZE (1 << 16)


enum {
  REG_DR,
  REG_ST_DMAWM,
  REG_ST_TIMEOUT,
  REG_FR,
  REG_LCRH_RX,
  REG_LCRH_TX,
  REG_IBRD,
  REG_FBRD,
  REG_CR,
  REG_IFLS,
  REG_IMSC,
  REG_RIS,
  REG_MIS,
  REG_ICR,
  REG_DMACR,
  REG_ST_XFCR,
  REG_ST_XON1,
  REG_ST_XON2,
  REG_ST_XOFF1,
  REG_ST_XOFF2,
  REG_ST_ITCR,
  REG_ST_ITIP,
  REG_ST_ABCR,
  REG_ST_ABIMSC,
  REG_ARRAY_SIZE,
};


static u16 _offsets[REG_ARRAY_SIZE] = {
  [REG_DR]      = UART01x_DR,
  [REG_FR]      = UART01x_FR,
  [REG_LCRH_RX] = UART011_LCRH,
  [REG_LCRH_TX] = UART011_LCRH,
  [REG_IBRD]    = UART011_IBRD,
  [REG_FBRD]    = UART011_FBRD,
  [REG_CR]      = UART011_CR,
  [REG_IFLS]    = UART011_IFLS,
  [REG_IMSC]    = UART011_IMSC,
  [REG_RIS]     = UART011_RIS,
  [REG_MIS]     = UART011_MIS,
  [REG_ICR]     = UART011_ICR,
  [REG_DMACR]   = UART011_DMACR,
};


static int _debug = 0;

struct ring_buf {
  unsigned char *buf;
  unsigned head;
  unsigned tail;
};

static struct {
  struct clk            *clk;
  struct ring_buf       tx_buf;
  struct ring_buf       rx_buf;
  spinlock_t            lock;
  unsigned              open;
  unsigned char __iomem *base;
  wait_queue_head_t     wait;
  unsigned long         req_events;
  unsigned              irq;
  unsigned              im;             // interrupt mask

  unsigned              rx_bytes;
  unsigned              tx_bytes;

  unsigned              brk_errs;
  unsigned              parity_errs;
  unsigned              frame_errs;
  unsigned              overruns;

  int                   major;
  struct class          *class;
  struct device         *dev;
} _port;


#define RING_BUF_INC(BUF, INDEX)                                        \
  do {(BUF).INDEX = ((BUF).INDEX + 1) & (BUF_SIZE - 1);} while (0)

#define RING_BUF_POP(BUF) RING_BUF_INC(BUF, tail)

#define RING_BUF_PUSH(BUF, C)                   \
  do {                                          \
    (BUF).buf[(BUF).head] = C;                  \
    RING_BUF_INC(BUF, head);                    \
  } while (0)

#define RING_BUF_POKE(BUF) (BUF).buf[(BUF).head]
#define RING_BUF_PEEK(BUF) (BUF).buf[(BUF).tail]

#define RING_BUF_MOVE(SRC, DST)                 \
  do {                                          \
    RING_BUF_PUSH(DST, RING_BUF_PEEK(SRC));     \
    RING_BUF_POP(SRC);                          \
  } while (0)


#define RING_BUF_SPACE(BUF) ((((BUF).tail) - ((BUF).head + 1)) & (BUF_SIZE - 1))
#define RING_BUF_FILL(BUF) ((((BUF).head) - ((BUF).tail)) & (BUF_SIZE - 1))


static unsigned _read(unsigned reg) {
  return readw_relaxed(_port.base + _offsets[reg]);
}


static void _write(unsigned val, unsigned reg) {
  writew_relaxed(val, _port.base + _offsets[reg]);
}


static unsigned _tx_chars(void) {
  unsigned bytes = 0;
  unsigned fill = RING_BUF_FILL(_port.tx_buf);

  for (int i = 0; i < fill; i++) {
    // Check if UART FIFO full
    if (_read(REG_FR) & UART01x_FR_TXFF) break;

    _write(RING_BUF_PEEK(_port.tx_buf), REG_DR);
    RING_BUF_POP(_port.tx_buf);
    mb();
    _port.tx_bytes++;
    bytes++;
  }

  // Stop TX when buffer is empty
  if (!RING_BUF_FILL(_port.tx_buf)) {
    _port.im &= ~UART011_TXIM;
    _write(_port.im, REG_IMSC);
  }

  return bytes;
}


static unsigned _rx_chars(void) {
  unsigned bytes = 0;
  unsigned space = RING_BUF_SPACE(_port.rx_buf);

  for (int i = 0; i < space; i++) {
    // Check if UART FIFO empty
    unsigned status = _read(REG_FR);
    if (status & UART01x_FR_RXFE) break;

    // Read char from FIFO and update status
    unsigned ch = _read(REG_DR);
    _port.rx_bytes++;

    // Record errors
    if (ch & UART011_DR_BE) _port.brk_errs++;
    if (ch & UART011_DR_PE) _port.parity_errs++;
    if (ch & UART011_DR_FE) _port.frame_errs++;
    if (ch & UART011_DR_OE) _port.overruns++;

    // Queue char
    RING_BUF_PUSH(_port.rx_buf, ch);
    bytes++;
  }

  // Stop RX interrupts when buffer is full
  if (!RING_BUF_SPACE(_port.rx_buf)) {
    _port.im &= ~(UART011_RXIM | UART011_RTIM | UART011_FEIM | UART011_PEIM |
                  UART011_BEIM | UART011_OEIM);
    _write(_port.im, REG_IMSC);
  }

  return bytes;
}


static irqreturn_t _interrupt(int irq, void *id) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  unsigned rBytes = 0;
  unsigned wBytes = 0;

  for (int pass = 0; pass < 256; pass++) {
    unsigned status = _read(REG_MIS);
    if (!status) break;

    // Clear interrupt status
    _write(status, REG_ICR);

    // Read and/or write
    if (status & (UART011_RTIS | UART011_RXIS)) rBytes += _rx_chars();
    if (status & UART011_TXIS) wBytes += _tx_chars();
  }

  spin_unlock_irqrestore(&_port.lock, flags);

  // Notify pollers
  if ((rBytes && (_port.req_events & POLLIN)) ||
      (wBytes && (_port.req_events & POLLOUT)))
    wake_up_interruptible(&_port.wait);

  return IRQ_HANDLED;
}


static void _enable_tx(void) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  _port.im |= UART011_TXIM;
  _write(_port.im, REG_IMSC);
  _tx_chars(); // Must prime the pump

  spin_unlock_irqrestore(&_port.lock, flags);
}


static void _enable_rx(void) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  _port.im |= UART011_RTIM | UART011_RXIM;
  _write(_port.im, REG_IMSC);

  spin_unlock_irqrestore(&_port.lock, flags);
}


static int _dev_open(struct inode *inodep, struct file *filep) {
  if (_debug) printk(KERN_INFO "bbserial: open()\n");
  if (_port.open) return -EBUSY;
  _port.open = 1;
  return 0;
}


static ssize_t _dev_read(struct file *filep, char *buffer, size_t len,
                         loff_t *offset) {
  if (_debug) printk(KERN_INFO "bbserial: read() len=%d\n", len);
  ssize_t bytes = 0;

  // TODO read whole blocks
  while (len && RING_BUF_FILL(_port.rx_buf)) {
    put_user(RING_BUF_PEEK(_port.rx_buf), buffer++);
    RING_BUF_POP(_port.rx_buf);
    len--;
    bytes++;
  }

  if (bytes) _enable_rx();

  return bytes ? bytes : -EAGAIN;
}


static ssize_t _dev_write(struct file *filep, const char *buffer, size_t len,
                         loff_t *offset) {
  if (_debug)
    printk(KERN_INFO "bbserial: write() len=%d tx=%d rx=%d\n",
           len, RING_BUF_FILL(_port.tx_buf), RING_BUF_FILL(_port.rx_buf));

  ssize_t bytes = 0;

  // TODO write whole blocks
  while (len && RING_BUF_SPACE(_port.tx_buf)) {
    get_user(RING_BUF_POKE(_port.tx_buf), buffer++);
    RING_BUF_INC(_port.tx_buf, head);
    len--;
    bytes++;
  }

  if (bytes) _enable_tx();

  return bytes ? bytes : -EAGAIN;
}


static int _dev_release(struct inode *inodep, struct file *filep) {
  printk(KERN_INFO "bbserial: release()\n");
  _port.open = 0;
  return 0;
}


static unsigned _dev_poll(struct file *file, poll_table *wait) {
  if (_debug) printk(KERN_INFO "bbserial: poll(tx=%d rx=%d)\n",
                     RING_BUF_FILL(_port.tx_buf), RING_BUF_FILL(_port.rx_buf));

  _port.req_events = poll_requested_events(wait);
  poll_wait(file, &_port.wait, wait);
  _port.req_events = 0;

  unsigned ret = 0;
  if (RING_BUF_SPACE(_port.tx_buf)) ret |= POLLOUT | POLLWRNORM;
  if (RING_BUF_FILL(_port.rx_buf))  ret |= POLLIN  | POLLRDNORM;

  return ret;
}


static long _dev_ioctl(struct file *file, unsigned cmd, unsigned long arg) {
  if (_debug) printk(KERN_INFO "bbserial: ioctl() cmd=%d arg=%lu\n", cmd, arg);

  int __user *ptr = (int __user *)arg;

  switch (cmd) {
  case TCGETS:   // TODO Get serial port settings
  case TCSETS:   // TODO Set serial port settings
  case TIOCMBIS: // TODO Set modem control state
    return 0;

  case TCFLSH: return 0;
  case FIONREAD: return put_user(RING_BUF_FILL(_port.rx_buf), ptr);
  default: return -ENOIOCTLCMD;
  }

  return 0;
}


static struct file_operations _ops = {
  .owner          = THIS_MODULE,
  .open           = _dev_open,
  .read           = _dev_read,
  .write          = _dev_write,
  .release        = _dev_release,
  .poll           = _dev_poll,
  .unlocked_ioctl = _dev_ioctl,
};


static int _probe(struct amba_device *dev, const struct amba_id *id) {
  if (_debug) printk(KERN_INFO "bbserial: probing\n");

  // Allocate buffers
  _port.tx_buf.buf = devm_kzalloc(&dev->dev, BUF_SIZE, GFP_KERNEL);
  _port.rx_buf.buf = devm_kzalloc(&dev->dev, BUF_SIZE, GFP_KERNEL);
  if (!_port.tx_buf.buf || !_port.rx_buf.buf) return -ENOMEM;

  // Map IO memory
  _port.base = devm_ioremap_resource(&dev->dev, &dev->res);
  if (IS_ERR(_port.base)) {
    dev_err(&dev->dev, "bbserial: failed to map IO memory\n");
   return PTR_ERR(_port.base);
  }

  // Get and enable clock
  _port.clk = devm_clk_get(&dev->dev, 0);
  if (IS_ERR(_port.clk)) {
    dev_err(&dev->dev, "bbserial: failed to get clock\n");
    return PTR_ERR(_port.clk);
  }

  int ret = clk_prepare_enable(_port.clk);
  if (ret) {
    dev_err(&dev->dev, "bbserial: clock prepare failed\n");
    return ret;
  }

  // Disable UART and mask interrupts
  _write(0, REG_CR);
  _write(0, REG_IMSC);

  // Set baud rate
  const unsigned baud = 230400;
  unsigned brd = clk_get_rate(_port.clk) * 16 / baud;
  if ((brd & 3) == 3) brd = (brd >> 2) + 1;
  else brd >>= 2;
  _write(brd & 0x3f, REG_FBRD);
  _write(brd >> 6,   REG_IBRD);

  // N81 & enable FIFOs
  _write(UART01x_LCRH_WLEN_8 | UART01x_LCRH_FEN, REG_LCRH_RX);

  // Enable, TX, RX, RTS, DTR & CTS
  unsigned cr = UART01x_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE |
    UART011_CR_RTS | UART011_CR_DTR | UART011_CR_CTSEN;
  _write(cr, REG_CR);

  // Set interrupt FIFO trigger levels
  _write(UART011_IFLS_RX2_8 | UART011_IFLS_TX6_8, REG_IFLS);

  // Clear pending interrupts
  _write(0x7ff, REG_ICR);

  // Enable read interrupts
  _port.im = _read(REG_IMSC);
  _enable_rx();

  // Allocate IRQ
  _port.irq = dev->irq[0];
  ret = request_irq(_port.irq, _interrupt, 0, "bbserial", &_port);

  if (ret) {
    dev_err(&dev->dev, "bbserial: request for IRQ failed\n");
    clk_disable_unprepare(_port.clk);
    return ret;
  }

  // Dynamically allocate device major number
  _port.major = register_chrdev(0, DEVICE_NAME, &_ops);
  if (_port.major < 0) {
    clk_disable_unprepare(_port.clk);
    dev_err(&dev->dev, "bbserial: failed to register a major number\n");
    return _port.major;
  }

  // Register device class
  _port.class = class_create(THIS_MODULE, "bbs");
  if (IS_ERR(_port.class)) {
    unregister_chrdev(_port.major, DEVICE_NAME);
    clk_disable_unprepare(_port.clk);
    dev_err(&dev->dev, "bbserial: ailed to register device class\n");
    return PTR_ERR(_port.class);
  }

  // Register device driver
  _port.dev =
    device_create(_port.class, 0, MKDEV(_port.major, 0), 0, DEVICE_NAME);
  if (IS_ERR(_port.dev)) {
    class_destroy(_port.class);
    unregister_chrdev(_port.major, DEVICE_NAME);
    dev_err(&dev->dev, "bbserial: failed to create the device\n");
    return PTR_ERR(_port.dev);
  }

  return 0;
}


static int _remove(struct amba_device *dev) {
  if (_debug) printk(KERN_INFO "bbserial: removing\n");

  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  // Mask and clear interrupts
  _write(0, REG_IMSC);
  _write(0x7ff, REG_ICR);

  // Disable UART
  _write(0, REG_CR);

  spin_unlock_irqrestore(&_port.lock, flags);

  // Free IRQ
  free_irq(_port.irq, &_port);
  synchronize_irq(_port.irq);

  // Shut down the clock producer
  clk_disable_unprepare(_port.clk);

  // Unload char dev
  device_destroy(_port.class, MKDEV(_port.major, 0));
  class_unregister(_port.class);
  class_destroy(_port.class);
  unregister_chrdev(_port.major, DEVICE_NAME);

  return 0;
}


static struct amba_id _ids[] = {
  {
    .id    = 0x00041011,
    .mask  = 0x000fffff,
    .data  = 0,
  },
  {0, 0},
};


MODULE_DEVICE_TABLE(amba, _ids);


static struct amba_driver _driver = {
  .drv = {.name  = "bbserial"},
  .id_table = _ids,
  .probe    = _probe,
  .remove   = _remove,
};


static int __init bbserial_init(void) {
  printk(KERN_INFO "bbserial: loaded\n");

  // Clear memory
  memset(&_port, 0, sizeof(_port));

  // Init lock
  spin_lock_init(&_port.lock);

  // Init wait queues
  init_waitqueue_head(&_port.wait);

  return amba_driver_register(&_driver);
}


static void __exit bbserial_exit(void) {
  amba_driver_unregister(&_driver);
  printk(KERN_INFO "bbserial: unloaded\n");
}


module_init(bbserial_init);
module_exit(bbserial_exit);
