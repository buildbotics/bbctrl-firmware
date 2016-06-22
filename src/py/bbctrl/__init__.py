#!/usr/bin/env python3

import os
import sys
import signal
from tornado import web, ioloop, escape
from sockjs.tornado import SockJSRouter, SockJSConnection
import json
import serial
import multiprocessing
import time
import select
import atexit
import argparse

from pkg_resources import Requirement, resource_filename

import lcd
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


def get_resource(path):
    return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


def on_exit(sig, func = None):
    print('exit handler triggered')
    sys.exit(1)


def encode_cmd(index, value, spec):
    if spec['type'] == 'enum': value = spec['values'].index(value)
    elif spec['type'] == 'bool': value = 1 if value else 0
    elif spec['type'] == 'percent': value /= 100.0

    cmd = '${}{}={}'.format(index, spec['code'], value)
    input_queue.put(cmd + '\n')
    #print(cmd)


def encode_config_category(index, config, category):
    for key, spec in category.items():
        if key in config:
            encode_cmd(index, config[key], spec)


def encode_config(index, config, tmpl):
    for category in tmpl.values():
        encode_config_category(index, config, category)


def update_config(config):
    # Motors
    tmpl = config_template['motors']
    for index in range(len(config['motors'])):
        encode_config(index + 1, config['motors'][index], tmpl)

    # Axes
    tmpl = config_template['axes']
    axes = 'xyzabc'
    for axis in axes:
        if not axis in config['axes']: continue
        encode_config(axis, config['axes'][axis], tmpl)

    # Switches
    tmpl = config_template['switches']
    for index in range(len(config['switches'])):
        encode_config_category(index + 1, config['switches'][index], tmpl)

    # Spindle
    tmpl = config_template['spindle']
    encode_config_category('', config['spindle'], tmpl)



class APIHandler(web.RequestHandler):
    def prepare(self):
        self.json = {}

        if self.request.body:
            try:
                self.json = escape.json_decode(self.request.body)
            except ValueError:
                self.send_error(400, message = 'Unable to parse JSON.')


    def set_default_headers(self):
        self.set_header('Content-Type', 'application/json')


    def write_error(self, status_code, **kwargs):
        e = {}
        e['message'] = str(kwargs['exc_info'][1])
        e['code'] = status_code

        self.write_json(e)


    def write_json(self, data):
        self.write(json.dumps(data))


class LoadHandler(APIHandler):
    def send_file(self, path):
        with open(path, 'r') as f:
            self.write_json(json.load(f))

    def get(self):
        try:
            self.send_file('config.json')
        except Exception as e:
            print(e)
            self.send_file(get_resource('http/default-config.json'))


class SaveHandler(APIHandler):
    def post(self):
        with open('config.json', 'w') as f:
            json.dump(self.json, f)

        update_config(self.json)
        print('Saved config')
        self.write_json('ok')


class FileHandler(APIHandler):
    def prepare(self): pass


    def delete(self, path):
        path = 'upload' + path
        if os.path.exists(path): os.unlink(path)
        self.write_json('ok')


    def put(self, path):
        path = 'upload' + path
        if not os.path.exists(path): return

        with open(path, 'r') as f:
            for line in f:
                input_queue.put(line)


    def get(self, path):
        if path:
            with open('upload/' + path, 'r') as f:
                self.write_json(f.read())
            return

        files = []

        if os.path.exists('upload'):
            for path in os.listdir('upload'):
                if os.path.isfile('upload/' + path):
                    files.append(path)

        self.write_json(files)


    def post(self, path):
        gcode = self.request.files['gcode'][0]

        if not os.path.exists('upload'): os.mkdir('upload')

        with open('upload/' + gcode['filename'], 'wb') as f:
            f.write(gcode['body'])

        self.write_json('ok')


class SerialProcess(multiprocessing.Process):
    def __init__(self, port, baud, input_queue, output_queue):
        multiprocessing.Process.__init__(self)
        self.input_queue = input_queue
        self.output_queue = output_queue
        self.sp = serial.Serial(port, baud, timeout = 1)
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
            fds = [self.input_queue._reader.fileno(), self.sp]
            ready = select.select(fds, [], [], 0.25)[0]

            # look for incoming tornado request
            if not self.input_queue.empty():
                data = self.input_queue.get()

                # send it to the serial device
                self.writeSerial(data)

            # look for incoming serial data
            if self.sp.inWaiting() > 0:
                try:
                    data = self.readSerial()
                    data = data.decode('utf-8').strip()
                    if not data: continue

                    # send it back to tornado
                    self.output_queue.put(data)

                    print(data)

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



# check the queue for pending messages, and relay them to all connected clients
def checkQueue():
    while not output_queue.empty():
        try:
            data = output_queue.get()
            msg = json.loads(data)
            state.update(msg)
            if clients: clients[0].broadcast(clients, msg)

        except Exception as e:
            print('ERROR: {}, data: {}'.format(e, data))


handlers = [
    (r'/api/load', LoadHandler),
    (r'/api/save', SaveHandler),
    (r'/api/file(/.*)?', FileHandler),
    (r'/(.*)', web.StaticFileHandler,
     {'path': os.path.join(DIR, get_resource('http/')),
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


def splash(screen):
    screen.clear()
    screen.display(0, 'Buildbotics', lcd.JUSTIFY_CENTER)
    screen.display(1, 'Controller', lcd.JUSTIFY_CENTER)
    screen.display(3, '*Ready*', lcd.JUSTIFY_CENTER)


def goodbye(screen):
    screen.clear()
    screen.display(1, 'Goodbye', lcd.JUSTIFY_CENTER)


def parse_args():
    parser = argparse.ArgumentParser(
        description = 'Buildbotics Machine Controller')

    parser.add_argument('-p', '--port', default = 80,
                        type = int, help = 'HTTP port')
    parser.add_argument('-a', '--addr', metavar = 'IP', default = '0.0.0.0',
                        help = 'HTTP address to bind')
    parser.add_argument('-s', '--serial', default = '/dev/ttyAMA0',
                        help = 'Serial device')
    parser.add_argument('-b', '--baud', default = 115200, type = int,
                        help = 'Serial baud rate')
    parser.add_argument('--lcd-port', default = 1, type = int,
                        help = 'LCD I2C port')
    parser.add_argument('--lcd-addr', default = 0x27, type = int,
                        help = 'LCD I2C address')
    parser.add_argument('-v', '--verbose', action = 'store_true',
                        help = 'Verbose output')

    return parser.parse_args()


def run():
    # Set signal handler
    signal.signal(signal.SIGTERM, on_exit)

    global args
    args = parse_args()

    # Load config template
    global config_template
    with open(get_resource('http/config-template.json'), 'r',
              encoding = 'utf-8') as f:
        config_template = json.load(f)

    # Init logging
    import logging
    logging.getLogger().setLevel(logging.DEBUG)

    # Start the serial worker
    try:
        sp = SerialProcess(args.serial, args.baud, input_queue, output_queue)
        sp.daemon = True
        sp.start()
    except Exception as e:
        print('Failed to open serial port:', e)

    # Adjust the interval according to frames sent by serial port
    ioloop.PeriodicCallback(checkQueue, 100).start()
    ioloop.PeriodicCallback(checkEvents, 100).start()

    # Setup LCD
    global screen
    screen = lcd.LCD(args.lcd_port, args.lcd_addr)
    splash(screen)
    atexit.register(goodbye, screen)

    # Start the web server
    app = web.Application(router.urls + handlers)

    try:
        app.listen(args.port, address = args.addr)
    except Exception as e:
        print('Failed to bind {}:{}:'.format(args.addr, args.port), e)
        sys.exit(1)

    print('Listening on http://{}:{}/'.format(args.addr, args.port))

    ioloop.IOLoop.instance().start()


if __name__ == "__main__": run()
