#!/usr/bin/env python3

## Change this to match your local settings
SERIAL_PORT = '/dev/ttyAMA0'
SERIAL_BAUDRATE = 115200
HTTP_PORT = 8080
HTTP_ADDR = '0.0.0.0'

import os
from tornado import web, ioloop
from sockjs.tornado import SockJSRouter, SockJSConnection
import json
import serial
import multiprocessing
import time

import inevent
from inevent.Constants import *


DIR = os.path.dirname(__file__)

config = {
    "deadband": 0.1,
    "axes": [ABS_X, ABS_Y, ABS_RZ, ABS_Z],
    "arrows": [ABS_HAT0X, ABS_HAT0Y],
    "speed": [0x120, 0x121, 0x122, 0x123],
    "activate": [0x124, 0x126, 0x125, 0x127],
    "verbose": False
    }

state = {}
clients = []

input_queue = multiprocessing.Queue()
output_queue = multiprocessing.Queue()


class SerialProcess(multiprocessing.Process):
    def __init__(self, input_queue, output_queue):
        multiprocessing.Process.__init__(self)
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.sp = serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout = 1)
        self.input_queue.put('\n')


    def close(self):
        self.sp.close()


    def writeSerial(self, data):
        self.sp.write(data.encode())


    def readSerial(self):
        return self.sp.readline().replace(b"\n", b"")


    def run(self):
        self.sp.flushInput()

        while True:
            # look for incoming tornado request
            if not self.input_queue.empty():
                data = self.input_queue.get()

                # send it to the serial device
                self.writeSerial(data)

            # look for incoming serial data
            if (self.sp.inWaiting() > 0):
                data = self.readSerial()
                # send it back to tornado
                self.output_queue.put(data)
                try:
                    print(data.decode('utf-8'))
                except Exception as e:
                    print(e, data)



class Connection(SockJSConnection):
    def on_open(self, info):
        clients.append(self)
        self.send(str.encode(json.dumps(state)))


    def on_close(self):
        clients.remove(self)


    def on_message(self, data):
        input_queue.put(data + '\n')



# check the queue for pending messages, and rely that to all connected clients
def checkQueue():
    while not output_queue.empty():
        try:
            msg = json.loads(output_queue.get())
            state.update(msg)
            for c in clients: c.send(msg)
        except: pass


handlers = [
    (r'/(.*)', web.StaticFileHandler,
     {'path': os.path.join(DIR, 'http/'),
      "default_filename": "index.html"}),
    ]

router = SockJSRouter(Connection, '/ws')


# Listen for input events
class JogHandler(inevent.JogHandler):
    def __init__(self, config):
        super().__init__(config)

        self.v = [0.0] * 4
        self.lastV = self.v


    def processed_events(self):
        if self.v != self.lastV:
            self.lastV = self.v

            v = ["{:6.5f}".format(x) for x in self.v]
            cmd = '$jog ' + ' '.join(v) + '\n'
            input_queue.put(cmd)


    def changed(self):
        if self.speed == 1: scale = 1.0 / 128.0
        if self.speed == 2: scale = 1.0 / 32.0
        if self.speed == 3: scale = 1.0 / 4.0
        if self.speed == 4: scale = 1.0

        self.v = [x * scale for x in self.axes]



def checkEvents():
    eventProcessor.process_events(eventHandler)
    eventHandler.processed_events()

eventProcessor = inevent.InEvent(types = "js kbd".split())
eventHandler = JogHandler(config)



if __name__ == "__main__":
    import logging
    logging.getLogger().setLevel(logging.DEBUG)

    # Start the serial worker
    try:
        sp = SerialProcess(input_queue, output_queue)
        sp.daemon = True
        sp.start()
    except Exception as e:
        print(e)

    # Adjust the interval according to frames sent by serial port
    ioloop.PeriodicCallback(checkQueue, 100).start()
    ioloop.PeriodicCallback(checkEvents, 100).start()

    # Start the web server
    app = web.Application(router.urls + handlers)
    app.listen(HTTP_PORT, address = HTTP_ADDR)
    print('Listening on http://{}:{}/'.format(HTTP_ADDR, HTTP_PORT))
    ioloop.IOLoop.instance().start()
