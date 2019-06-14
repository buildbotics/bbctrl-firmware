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
#include <linux/tty.h>
#include <asm/ioctls.h>
#include <asm/termios.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joseph Coffland");
MODULE_DESCRIPTION("Buildbotics controller serial port driver");
MODULE_VERSION("0.3");


#define DEVICE_NAME "ttyAMA0"
#define BUF_SIZE (1 << 16)


#define UART01x_LCRH_WLEN_bm 0x60


static int debug = 0;
module_param(debug, int, 0660);


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
  wait_queue_head_t     read_wait;
  wait_queue_head_t     write_wait;
  unsigned              irq;
  unsigned              im;             // interrupt mask

  unsigned              brk_errs;
  unsigned              parity_errs;
  unsigned              frame_errs;
  unsigned              overruns;

  int                   major;
  struct class          *class;
  struct device         *dev;
  struct ktermios       term;
} _port;


#define RING_BUF_INC(BUF, INDEX)                                        \
  do {(BUF).INDEX = ((BUF).INDEX + 1) & (BUF_SIZE - 1);} while (0)

#define RING_BUF_POP(BUF) RING_BUF_INC(BUF, tail)

#define RING_BUF_PUSH(BUF, C)                   \
  do {                                          \
    (BUF).buf[(BUF).head] = C;                  \
    mb();                                       \
    RING_BUF_INC(BUF, head);                    \
  } while (0)

#define RING_BUF_POKE(BUF) (BUF).buf[(BUF).head]
#define RING_BUF_PEEK(BUF) (BUF).buf[(BUF).tail]
#define RING_BUF_SPACE(BUF) ((((BUF).tail) - ((BUF).head + 1)) & (BUF_SIZE - 1))
#define RING_BUF_FILL(BUF) ((((BUF).head) - ((BUF).tail)) & (BUF_SIZE - 1))
#define RING_BUF_CLEAR(BUF) do {(BUF).head = (BUF).tail = 0;} while (0)


static unsigned _read(unsigned reg) {return readw_relaxed(_port.base + reg);}


static void _write(unsigned val, unsigned reg) {
  writew_relaxed(val, _port.base + reg);
}


static void _tx_chars(void) {
  unsigned fill = RING_BUF_FILL(_port.tx_buf);

  while (fill--) {
    // Check if UART FIFO full
    if (_read(UART01x_FR) & UART01x_FR_TXFF) break;

    _write(RING_BUF_PEEK(_port.tx_buf), UART01x_DR);
    mb();
    RING_BUF_POP(_port.tx_buf);
  }

  // Stop TX when buffer is empty
  if (!RING_BUF_FILL(_port.tx_buf)) {
    _port.im &= ~UART011_TXIM;
    _write(_port.im, UART011_IMSC);
  }
}


static void _rx_chars(void) {
  unsigned space = RING_BUF_SPACE(_port.rx_buf);

  while (space--) {
    // Check if UART FIFO empty
    unsigned status = _read(UART01x_FR);
    if (status & UART01x_FR_RXFE) break;

    // Read char from FIFO and update status
    unsigned ch = _read(UART01x_DR);

    // Record errors
    if (ch & UART011_DR_BE) _port.brk_errs++;
    if (ch & UART011_DR_PE) _port.parity_errs++;
    if (ch & UART011_DR_FE) _port.frame_errs++;
    if (ch & UART011_DR_OE) _port.overruns++;

    // Queue char
    RING_BUF_PUSH(_port.rx_buf, ch);
  }

  // Stop RX interrupts when buffer is full
  if (!RING_BUF_SPACE(_port.rx_buf)) {
    _port.im &= ~(UART011_RXIM | UART011_RTIM);
    _write(_port.im, UART011_IMSC);
  }
}


static int _read_status(void) {
  int status = 0;

  unsigned fr = _read(UART01x_FR);
  unsigned cr = _read(UART011_CR);

  if (fr & UART01x_FR_DSR) status |= TIOCM_LE;  // DSR (data set ready)
  if (cr & UART011_CR_DTR) status |= TIOCM_DTR; // DTR (data terminal ready)
  if (cr & UART011_CR_RTS) status |= TIOCM_RTS; // RTS (request to send)
  // TODO What is TIOCM_ST - Secondary TXD (transmit)?
  // TODO What is TIOCM_SR - Secondary RXD (receive)?
  if (fr & UART01x_FR_CTS) status |= TIOCM_CTS; // CTS (clear to send)
  if (fr & UART01x_FR_DCD) status |= TIOCM_CD;  // DCD (data carrier detect)
  if (fr & UART011_FR_RI)  status |= TIOCM_RI;  // RI  (ring)
  if (fr & UART01x_FR_DSR) status |= TIOCM_DSR; // DSR (data set ready)

  if (debug) printk(KERN_INFO "bbserial: _read_status() = %d\n", status);

  return status;
}


static void _write_status(int status) {
  if (debug) printk(KERN_INFO "bbserial: _write_status() = %d\n", status);

  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  unsigned cr = _read(UART011_CR);

  // DTR (data terminal ready)
  if (status & TIOCM_DTR) cr |= UART011_CR_DTR;
  else cr &= ~UART011_CR_DTR;

  // RTS (request to send)
  if (status & TIOCM_RTS) cr |= UART011_CR_RTS;
  else cr &= ~UART011_CR_RTS;

  _write(cr, UART011_CR);

  spin_unlock_irqrestore(&_port.lock, flags);
}


static struct ktermios *_get_term(void) {
  unsigned lcrh = _read(UART011_LCRH);
  unsigned cr = _read(UART011_CR);

  // Baud rate
  unsigned brd = _read(UART011_IBRD) << 6 | _read(UART011_FBRD);
  speed_t baud = clk_get_rate(_port.clk) * 4 / brd;
  tty_termios_encode_baud_rate(&_port.term, baud, baud);

  // Data bits
  unsigned cflag;
  switch (lcrh & UART01x_LCRH_WLEN_bm) {
  case UART01x_LCRH_WLEN_5: cflag = CS5; break;
  case UART01x_LCRH_WLEN_6: cflag = CS6; break;
  case UART01x_LCRH_WLEN_7: cflag = CS7; break;
  default: cflag = CS8; break;
  }

  // Stop bits
  if (lcrh & UART01x_LCRH_STP2) cflag |= CSTOPB;

  // Parity
  if (lcrh & UART01x_LCRH_PEN) {
    cflag |= PARENB;

    if (!(UART01x_LCRH_EPS & lcrh)) cflag |= PARODD;
    if (UART011_LCRH_SPS & lcrh) cflag |= CMSPAR;
  }

  // Hardware flow control
  if (cr & UART011_CR_CTSEN) cflag |= CRTSCTS;

  _port.term.c_cflag = cflag;

  return &_port.term;
}


static void _set_baud(speed_t baud) {
  if (debug) printk(KERN_INFO "bbserial: baud=%d\n", baud);

  unsigned brd = clk_get_rate(_port.clk) * 16 / baud;

  if ((brd & 3) == 3) brd = (brd >> 2) + 1; // Round up
  else brd >>= 2;

  _write(brd & 0x3f, UART011_FBRD);
  _write(brd >> 6,   UART011_IBRD);
}


static int _set_term(struct ktermios *term) {
  unsigned lcrh = UART01x_LCRH_FEN; // Enable FIFOs
  unsigned cflag = term->c_cflag;

  // Data bits
  switch (cflag & CSIZE) {
  case CS5: lcrh |= UART01x_LCRH_WLEN_5; break;
  case CS6: lcrh |= UART01x_LCRH_WLEN_6; break;
  case CS7: lcrh |= UART01x_LCRH_WLEN_7; break;
  default:  lcrh |= UART01x_LCRH_WLEN_8; break;
  }

  // Stop bits
  if (cflag & CSTOPB) lcrh |= UART01x_LCRH_STP2;

  // Parity
  if (cflag & PARENB) {
    lcrh |= UART01x_LCRH_PEN;

    if (!(cflag & PARODD)) lcrh |= UART01x_LCRH_EPS;
    if (cflag & CMSPAR) lcrh |= UART011_LCRH_SPS;
  }

  // Get baud rate
  speed_t baud = tty_termios_baud_rate(term);

  // Set
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  // Hardware flow control
  unsigned cr = _read(UART011_CR);
  if (cflag & CRTSCTS) cr |= UART011_CR_CTSEN;

  _write(0, UART011_CR);      // Disable
  _set_baud(baud);            // Baud
  _write(lcrh, UART011_LCRH); // Must be after baud
  _write(cr, UART011_CR);     // Enable

  spin_unlock_irqrestore(&_port.lock, flags);

  return 0;
}


static void _flush_input(void) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  RING_BUF_CLEAR(_port.rx_buf);

  spin_unlock_irqrestore(&_port.lock, flags);
}


static void _flush_output(void) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  RING_BUF_CLEAR(_port.tx_buf);

  spin_unlock_irqrestore(&_port.lock, flags);
}


static irqreturn_t _interrupt(int irq, void *id) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  // Read and/or write
  unsigned status = _read(UART011_MIS);
  if (status & (UART011_RTIS | UART011_RXIS)) _rx_chars();
  if (status & UART011_TXIS) _tx_chars();

  unsigned txSpace = RING_BUF_SPACE(_port.tx_buf);
  unsigned rxFill  = RING_BUF_FILL(_port.rx_buf);

  spin_unlock_irqrestore(&_port.lock, flags);

  // Notify pollers
  if (rxFill)  wake_up_interruptible_poll(&_port.read_wait,  POLLIN);
  if (txSpace) wake_up_interruptible_poll(&_port.write_wait, POLLOUT);

  return IRQ_HANDLED;
}


static void _enable_tx(void) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  _port.im |= UART011_TXIM;
  _write(_port.im, UART011_IMSC);
  _tx_chars(); // Must prime the pump

  spin_unlock_irqrestore(&_port.lock, flags);
}


static int _tx_enabled(void) {return _port.im & UART011_TXIM;}


static void _enable_rx(void) {
  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  _port.im |= UART011_RTIM | UART011_RXIM;
  _write(_port.im, UART011_IMSC);

  spin_unlock_irqrestore(&_port.lock, flags);
}


static int _rx_enabled(void) {return _port.im & (UART011_RTIM | UART011_RXIM);}


static int _dev_open(struct inode *inodep, struct file *filep) {
  if (debug) printk(KERN_INFO "bbserial: open()\n");
  if (_port.open) return -EBUSY;
  _port.open = 1;
  return 0;
}


static ssize_t _dev_read(struct file *filep, char *buffer, size_t len,
                         loff_t *offset) {
  if (debug) printk(KERN_INFO "bbserial: read() len=%d overruns=%d\n", len,
                    _port.overruns);

  ssize_t bytes = 0;

  while (bytes < len && RING_BUF_FILL(_port.rx_buf)) {
    put_user(RING_BUF_PEEK(_port.rx_buf), buffer++);
    RING_BUF_POP(_port.rx_buf);
    bytes++;
    if (!_rx_enabled()) _enable_rx();
  }

  return bytes ? bytes : -EAGAIN;
}


static ssize_t _dev_write(struct file *filep, const char *buffer, size_t len,
                          loff_t *offset) {
  if (debug)
    printk(KERN_INFO "bbserial: write() len=%d tx=%d rx=%d\n",
           len, RING_BUF_FILL(_port.tx_buf), RING_BUF_FILL(_port.rx_buf));

  ssize_t bytes = 0;

  while (bytes < len && RING_BUF_SPACE(_port.tx_buf)) {
    get_user(RING_BUF_POKE(_port.tx_buf), buffer++);
    RING_BUF_INC(_port.tx_buf, head);
    bytes++;
    if (!_tx_enabled()) _enable_tx();
  }

  return bytes ? bytes : -EAGAIN;
}


static int _dev_release(struct inode *inodep, struct file *filep) {
  printk(KERN_INFO "bbserial: release()\n");
  _port.open = 0;
  return 0;
}


static unsigned _dev_poll(struct file *file, poll_table *wait) {
  if (debug) {
    unsigned events = poll_requested_events(wait);
    printk(KERN_INFO "bbserial: poll(in=%s, out=%s)\n",
           (events & POLLIN)  ? "true" : "false",
           (events & POLLOUT) ? "true" : "false");
  }

  poll_wait(file, &_port.read_wait,  wait);
  poll_wait(file, &_port.write_wait, wait);

  unsigned ret = 0;
  if (RING_BUF_FILL(_port.rx_buf))  ret |= POLLIN  | POLLRDNORM;
  if (RING_BUF_SPACE(_port.tx_buf)) ret |= POLLOUT | POLLWRNORM;

  if (debug) printk(KERN_INFO "bbserial: tx=%d rx=%d\n",
                     RING_BUF_FILL(_port.tx_buf), RING_BUF_FILL(_port.rx_buf));

  return ret;
}


static long _dev_ioctl(struct file *file, unsigned cmd, unsigned long arg) {
  if (debug)
    printk(KERN_INFO "bbserial: ioctl() cmd=0x%04x arg=%lu\n", cmd, arg);

  int __user *ptr = (int __user *)arg;
  int status;

  switch (cmd) {
  case TCGETS: { // Get serial port settings
    struct ktermios *term = _get_term();
    if (copy_to_user((void __user *)arg, &term, sizeof(struct termios)))
      return -EFAULT;
    return 0;
  }

  case TCSETS: { // Set serial port settings
    struct ktermios term;
    if (copy_from_user(&term, (void __user *)arg, sizeof(struct termios)))
      return -EFAULT;
    return _set_term(&term);
  }

  case TIOCMGET: // Get status of modem bits
    put_user(_read_status(), ptr);
    return 0;

  case TIOCMSET: // Set status of modem bits
    get_user(status, ptr);
    _write_status(status);
    return 0;

  case TIOCMBIC: // Clear indicated modem bits
    get_user(status, ptr);
    _write_status(~status & _read_status());
    return 0;

  case TIOCMBIS: // Set indicated modem bits
    get_user(status, ptr);
    _write_status(status | _read_status());
    return 0;

  case TCFLSH: // Flush
    if (arg == TCIFLUSH || arg == TCIOFLUSH) _flush_input();
    if (arg == TCOFLUSH || arg == TCIOFLUSH) _flush_output();
    return 0;

  case TIOCINQ:  return put_user(RING_BUF_FILL(_port.rx_buf), ptr);
  case TIOCOUTQ: return put_user(RING_BUF_FILL(_port.tx_buf), ptr);

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
  if (debug) printk(KERN_INFO "bbserial: probing\n");

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
  _write(0, UART011_CR);
  _write(0, UART011_IMSC);

  // Set default baud rate
  _set_baud(38400);

  // N81 & enable FIFOs, must be after baud
  _write(UART01x_LCRH_WLEN_8 | UART01x_LCRH_FEN, UART011_LCRH);

  // Enable, TX, RX, RTS, DTR & CTS
  unsigned cr = UART01x_CR_UARTEN | UART011_CR_RXE | UART011_CR_TXE |
    UART011_CR_RTS | UART011_CR_DTR | UART011_CR_CTSEN;
  _write(cr, UART011_CR);

  // Set interrupt FIFO trigger levels
  _write(UART011_IFLS_RX2_8 | UART011_IFLS_TX6_8, UART011_IFLS);

  // Clear pending interrupts
  _write(0x7ff, UART011_ICR);

  // Enable read interrupts
  _port.im = 0;
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
    dev_err(&dev->dev, "bbserial: failed to register device class\n");
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
  if (debug) printk(KERN_INFO "bbserial: removing\n");

  unsigned long flags;
  spin_lock_irqsave(&_port.lock, flags);

  // Mask and clear interrupts
  _write(0, UART011_IMSC);
  _write(0x7ff, UART011_ICR);

  // Disable UART
  _write(0, UART011_CR);

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
  init_waitqueue_head(&_port.read_wait);
  init_waitqueue_head(&_port.write_wait);

  return amba_driver_register(&_driver);
}


static void __exit bbserial_exit(void) {
  amba_driver_unregister(&_driver);
  printk(KERN_INFO "bbserial: unloaded\n");
}


module_init(bbserial_init);
module_exit(bbserial_exit);
