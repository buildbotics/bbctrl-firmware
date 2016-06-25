import re
import serial
import logging


log = logging.getLogger('AVR')


class AVR():
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.state = 'idle'
        self.line = -1
        self.step = 0
        self.f = None

        try:
            self.sp = serial.Serial(ctrl.args.serial, ctrl.args.baud,
                                    rtscts = 1, timeout = 0)

        except Exception as e:
            log.warning('Failed to open serial port: %s', e)
            return

        self.in_buf = ''
        self.out_buf = None
        self.ctrl.input_queue.put('$echo=0\n\n')

        ctrl.ioloop.add_handler(self.sp, self.serial_handler, ctrl.ioloop.READ)
        ctrl.ioloop.add_handler(self.ctrl.input_queue._reader.fileno(),
                                self.queue_handler, ctrl.ioloop.READ)


    def close(self):
        self.sp.close()


    def set_write(self, enable):
        flags = self.ctrl.ioloop.READ
        if enable: flags |= self.ctrl.ioloop.WRITE
        self.ctrl.ioloop.update_handler(self.sp, flags)


    def serial_handler(self, fd, events):
        if self.ctrl.ioloop.READ & events: self.serial_read()
        if self.ctrl.ioloop.WRITE & events: self.serial_write()


    def serial_write(self):
        # Finish writing current line
        if self.out_buf is not None:
            try:
                count = self.sp.write(self.out_buf)
                log.debug('Wrote %d', count)
            except Exception as e:
                self.set_write(False)
                raise e

            self.out_buf = self.out_buf[count:]
            if len(self.out_buf): return
            self.out_buf = None

        # Close file if stopped
        if self.state == 'stop' and self.f is not None:
            self.f.close()
            self.f = None

        # Read next line if running
        if self.state == 'run':
            # Strip white-space & comments and encode to bytearray
            self.out_buf = self.f.readline().strip()
            self.out_buf = re.sub(r';.*', '', self.out_buf)
            if len(self.out_buf):
                log.info(self.out_buf)
                self.out_buf = bytes(self.out_buf + '\n', 'utf-8')
            else: self.out_buf = None

            # Pause if done stepping
            if self.step:
                self.step -= 1
                if not self.step:
                    self.state = 'pause'

        # Stop if not longer running
        else:
            self.set_write(False)
            self.step = 0


    def serial_read(self):
        try:
            data = self.sp.read(self.sp.inWaiting())
            self.in_buf += data.decode('utf-8')

        except Exception as e:
            log.warning('%s: %s', e, data)

        # Parse incoming serial data into lines
        while True:
            i = self.in_buf.find('\n')
            if i == -1: break
            line = self.in_buf[0:i].strip()
            self.in_buf = self.in_buf[i + 1:]

            if line:
                self.ctrl.output_queue.put(line)
                log.debug(line)


    def queue_handler(self, fd, events):
        if self.ctrl.input_queue.empty(): return

        data = self.ctrl.input_queue.get()
        self.sp.write(data.encode())


    def home(self):
        if self.state != 'idle': raise Exception('Already running')
        # TODO


    def start(self, path):
        if self.f is None:
            self.f = open('upload' + path, 'r')
            self.line = 0

        self.set_write(True)
        self.state = 'run'


    def stop(self):
        if self.state == 'idle': return
        self.state == 'stop'


    def pause(self, optional):
        self.state = 'pause'


    def step(self):
        self.step += 1
        if self.state == 'idle': self.start()
        else: self.state = 'run'
