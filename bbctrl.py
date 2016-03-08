#!/usr/bin/env python

## Change this to match your local settings
SERIAL_PORT = '/dev/ttyAMA0'
SERIAL_BAUDRATE = 115200

import os
from tornado import web, ioloop
from sockjs.tornado import SockJSRouter, SockJSConnection
import json
import serial
import multiprocessing

DIR = os.path.dirname(__file__)

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
        return self.sp.readline().replace("\n", "")


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
                print data



class Connection(SockJSConnection):
    def on_open(self, info):
        clients.append(self)
        self.send(state)

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


if __name__ == "__main__":
    import logging
    logging.getLogger().setLevel(logging.DEBUG)

    # Start the serial worker
    sp = SerialProcess(input_queue, output_queue)
    sp.daemon = True
    sp.start()

    # Adjust the interval according to frames sent by serial port
    ioloop.PeriodicCallback(checkQueue, 100).start()

    # Start the web server
    app = web.Application(router.urls + handlers)
    app.listen(8080)
    ioloop.IOLoop.instance().start()
