import os
import sys
import json
import multiprocessing
import tornado
import sockjs.tornado
import logging

import bbctrl


log = logging.getLogger('Web')


class APIHandler(tornado.web.RequestHandler):
    def prepare(self):
        self.json = {}

        if self.request.body:
            try:
                self.json = tornado.escape.json_decode(self.request.body)
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
            log.warning('%s', e)
            self.send_file(bbctrl.get_resource('default-config.json'))



class SaveHandler(APIHandler):
    def post(self):
        with open('config.json', 'w') as f:
            json.dump(self.json, f)

        self.application.update_config(self.json)
        log.info('Saved config')
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
                self.application.input_queue.put(line)


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



class Connection(sockjs.tornado.SockJSConnection):
    def heartbeat(self):
        self.timer = self.app.ioloop.call_later(3, self.heartbeat)
        self.send_json({'heartbeat': self.count})
        self.count += 1


    def send_json(self, data):
        self.send(str.encode(json.dumps(data)))


    def on_open(self, info):
        self.app = self.session.server.app
        self.timer = self.app.ioloop.call_later(3, self.heartbeat)
        self.count = 0;
        self.app.clients.append(self)
        self.send_json(self.session.server.app.state)


    def on_close(self):
        self.app.ioloop.remove_timeout(self.timer)
        self.app.clients.remove(self)


    def on_message(self, data):
        self.app.input_queue.put(data + '\n')



class Web(tornado.web.Application):
    def __init__(self, addr, port, ioloop):
        # Load config template
        with open(bbctrl.get_resource('http/config-template.json'), 'r',
                  encoding = 'utf-8') as f:
            self.config_template = json.load(f)

        self.ioloop = ioloop
        self.state = {}
        self.clients = []

        self.input_queue = multiprocessing.Queue()
        self.output_queue = multiprocessing.Queue()

        # Handle output queue events
        ioloop.add_handler(self.output_queue._reader.fileno(),
                           self.queue_handler, ioloop.READ)

        handlers = [
            (r'/api/load', LoadHandler),
            (r'/api/save', SaveHandler),
            (r'/api/file(/.*)?', FileHandler),
            (r'/(.*)', tornado.web.StaticFileHandler,
             {'path': bbctrl.get_resource('http/'),
              "default_filename": "index.html"}),
            ]

        router = sockjs.tornado.SockJSRouter(Connection, '/ws')
        router.app = self

        tornado.web.Application.__init__(self, router.urls + handlers)

        try:
            self.listen(port, address = addr)

        except Exception as e:
            log.error('Failed to bind %s:%d: %s', addr, port, e)
            sys.exit(1)

        log.info('Listening on http://%s:%d/', addr, port)


    def queue_handler(self, fd, events):
        try:
            data = self.output_queue.get()
            msg = json.loads(data)
            self.state.update(msg)
            if self.clients:
                self.clients[0].broadcast(self.clients, msg)

        except Exception as e:
            log.error('%s, data: %s', e, data)


    def encode_cmd(self, index, value, spec):
        if spec['type'] == 'enum': value = spec['values'].index(value)
        elif spec['type'] == 'bool': value = 1 if value else 0
        elif spec['type'] == 'percent': value /= 100.0

        cmd = '${}{}={}'.format(index, spec['code'], value)
        self.input_queue.put(cmd + '\n')
        #log.info(cmd)


    def encode_config_category(self, index, config, category):
        for key, spec in category.items():
            if key in config:
                self.encode_cmd(index, config[key], spec)


    def encode_config(self, index, config, tmpl):
        for category in tmpl.values():
            self.encode_config_category(index, config, category)


    def update_config(self, config):
        # Motors
        tmpl = self.config_template['motors']
        for index in range(len(config['motors'])):
            self.encode_config(index + 1, config['motors'][index], tmpl)

        # Axes
        tmpl = self.config_template['axes']
        axes = 'xyzabc'
        for axis in axes:
            if not axis in config['axes']: continue
            self.encode_config(axis, config['axes'][axis], tmpl)

        # Switches
        tmpl = self.config_template['switches']
        for index in range(len(config['switches'])):
            self.encode_config_category(index + 1,
                                        config['switches'][index], tmpl)

        # Spindle
        tmpl = self.config_template['spindle']
        self.encode_config_category('', config['spindle'], tmpl)
