import os
import sys
import json
import tornado
import sockjs.tornado
import logging
import datetime
import shutil
import tarfile

import bbctrl


log = logging.getLogger('Web')



class ConfigLoadHandler(bbctrl.APIHandler):
    def get(self): self.write_json(self.ctrl.config.load())


class ConfigDownloadHandler(bbctrl.APIHandler):
    def set_default_headers(self):
        filename = datetime.date.today().strftime('bbctrl-%Y%m%d.json')
        self.set_header('Content-Type', 'application/octet-stream')
        self.set_header('Content-Disposition',
                        'attachment; filename="%s"' % filename)

    def get(self):
        self.write_json(self.ctrl.config.load(), pretty = True)


class ConfigSaveHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.config.save(self.json)


class ConfigResetHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.config.reset()


class FirmwareUpdateHandler(bbctrl.APIHandler):
    def prepare(self): pass


    def put(self):
        # Only allow this function in dev mode
        if not os.path.exists('/etc/bbctrl-dev-mode'):
            self.send_error(403, message = 'Not in dev mode')
            return

        firmware = self.request.files['firmware'][0]

        if not os.path.exists('firmware'): os.mkdir('firmware')

        with open('firmware/update.tar.bz2', 'wb') as f:
            f.write(firmware['body'])

        import subprocess
        ret = subprocess.Popen(['update-bbctrl'])

        self.write_json('ok')


class UpgradeHandler(bbctrl.APIHandler):
    def put_ok(self):
        import subprocess
        ret = subprocess.Popen(['upgrade-bbctrl'])


class HomeHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.home()


class StartHandler(bbctrl.APIHandler):
    def put_ok(self, path): self.ctrl.avr.start(path)


class EStopHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.estop()


class ClearHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.clear()


class StopHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.stop()


class PauseHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.pause()


class UnpauseHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.unpause()


class OptionalPauseHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.optional_pause()


class StepHandler(bbctrl.APIHandler):
    def put_ok(self, path): self.ctrl.avr.step(path)


class ZeroHandler(bbctrl.APIHandler):
    def put_ok(self, axis):
        if axis is not None: axis = ord(axis[1:].lower())
        self.ctrl.avr.zero(axis)


class OverrideFeedHandler(bbctrl.APIHandler):
    def put_ok(self, value): self.ctrl.avr.override_feed(float(value))


class OverrideSpeedHandler(bbctrl.APIHandler):
    def put_ok(self, value): self.ctrl.avr.override_speed(float(value))


class WSConnection(tornado.websocket.WebSocketHandler):
    def __init__(self, app, request, **kwargs):
        super(WSConnection, self).__init__(app, request, **kwargs)
        self.ctrl = app.ctrl
        self.timer = None


    def heartbeat(self):
        self.timer = self.ctrl.ioloop.call_later(3, self.heartbeat)
        self.write_message({'heartbeat': self.count})
        self.count += 1


    def open(self):
        self.clients = self.ctrl.web.ws_clients

        self.timer = self.ctrl.ioloop.call_later(3, self.heartbeat)
        self.count = 0;

        self.clients.append(self)
        self.write_message(self.ctrl.avr.vars)


    def on_close(self):
        if self.timer is not None: self.ctrl.ioloop.remove_timeout(self.timer)
        self.clients.remove(self)


    def on_message(self, msg): pass


class SockJSConnection(sockjs.tornado.SockJSConnection):
    def heartbeat(self):
        self.timer = self.ctrl.ioloop.call_later(3, self.heartbeat)
        self.send({'heartbeat': self.count})
        self.count += 1


    def on_open(self, info):
        self.ctrl = self.session.server.ctrl
        self.clients = self.ctrl.web.sockjs_clients

        self.timer = self.ctrl.ioloop.call_later(3, self.heartbeat)
        self.count = 0;

        self.clients.append(self)
        self.send(self.ctrl.avr.vars)


    def on_close(self):
        self.ctrl.ioloop.remove_timeout(self.timer)
        self.clients.remove(self)


    def on_message(self, data):
        self.ctrl.avr.mdi(data)


class StaticFileHandler(tornado.web.StaticFileHandler):
    def set_extra_headers(self, path):
        self.set_header('Cache-Control',
                        'no-store, no-cache, must-revalidate, max-age=0')


class Web(tornado.web.Application):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.ws_clients = []
        self.sockjs_clients = []

        handlers = [
            (r'/websocket', WSConnection),
            (r'/api/config/load', ConfigLoadHandler),
            (r'/api/config/download', ConfigDownloadHandler),
            (r'/api/config/save', ConfigSaveHandler),
            (r'/api/config/reset', ConfigResetHandler),
            (r'/api/firmware/update', FirmwareUpdateHandler),
            (r'/api/upgrade', UpgradeHandler),
            (r'/api/file(/.+)?', bbctrl.FileHandler),
            (r'/api/home', HomeHandler),
            (r'/api/start(/.+)', StartHandler),
            (r'/api/estop', EStopHandler),
            (r'/api/clear', ClearHandler),
            (r'/api/stop', StopHandler),
            (r'/api/pause', PauseHandler),
            (r'/api/unpause', UnpauseHandler),
            (r'/api/pause/optional', OptionalPauseHandler),
            (r'/api/step(/.+)', StepHandler),
            (r'/api/zero(/[xyzabcXYZABC])?', ZeroHandler),
            (r'/api/override/feed/([\d.]+)', OverrideFeedHandler),
            (r'/api/override/speed/([\d.]+)', OverrideSpeedHandler),
            (r'/(.*)', StaticFileHandler,
             {'path': bbctrl.get_resource('http/'),
              "default_filename": "index.html"}),
            ]

        router = sockjs.tornado.SockJSRouter(SockJSConnection, '/sockjs')
        router.ctrl = ctrl

        tornado.web.Application.__init__(self, router.urls + handlers)

        try:
            self.listen(ctrl.args.port, address = ctrl.args.addr)

        except Exception as e:
            log.error('Failed to bind %s:%d: %s', ctrl.args.addr,
                      ctrl.args.port, e)
            sys.exit(1)

        log.info('Listening on http://%s:%d/', ctrl.args.addr, ctrl.args.port)


    def broadcast(self, msg):
        if len(self.sockjs_clients):
            self.sockjs_clients[0].broadcast(self.sockjs_clients, msg)

        for client in self.ws_clients: client.write_message(msg)
