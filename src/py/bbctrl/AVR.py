import serial
import logging


log = logging.getLogger('AVR')


class AVR():
    def __init__(self, port, baud, ioloop, app):
        self.app = app

        try:
            self.sp = serial.Serial(port, baud, timeout = 1)

        except Exception as e:
            log.warning('Failed to open serial port: %s', e)
            return

        self.in_buf = ''
        self.app.input_queue.put('\n')

        ioloop.add_handler(self.sp, self.serial_handler, ioloop.READ)
        ioloop.add_handler(self.app.input_queue._reader.fileno(),
                           self.queue_handler, ioloop.READ)


    def close(self):
        self.sp.close()


    def serial_handler(self, fd, events):
        try:
            data = self.sp.read(self.sp.inWaiting())
            self.in_buf += data.decode('utf-8')

        except Exception as e:
            log.warning('%s: %s', e, data)

        while True:
            i = self.in_buf.find('\n')
            if i == -1: break
            line = self.in_buf[0:i].strip()
            self.in_buf = self.in_buf[i + 1:]

            if line:
                self.app.output_queue.put(line)
                log.debug(line)


    def queue_handler(self, fd, events):
        if self.app.input_queue.empty(): return

        data = self.app.input_queue.get()
        self.sp.write(data.encode())
