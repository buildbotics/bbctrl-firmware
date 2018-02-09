import os
import sys
import json
import tornado
import sockjs.tornado
import logging
import datetime
import shutil
import tarfile
import subprocess
import socket
from tornado.web import HTTPError

import bbctrl


log = logging.getLogger('Web')


def call_get_output(cmd):
    p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    s = p.communicate()[0].decode('utf-8')
    if p.returncode: raise HTTPError(400, 'Command failed')
    return s


def get_username():
    return call_get_output(['getent', 'passwd', '1001']).split(':')[0]


def set_username(username):
    if subprocess.call(['usermod', '-l', username, get_username()]):
        raise HTTPError(400, 'Failed to set username to "%s"' % username)


def check_password(password):
    # Get current password
    s = call_get_output(['getent', 'shadow', get_username()])
    current = s.split(':')[1].split('$')

    # Check password type
    if current[1] != '1':
        raise HTTPError(400, "Don't know how to update non-MD5 password")

    # Check current password
    cmd = ['openssl', 'passwd', '-salt', current[2], '-1', password]
    s = call_get_output(cmd).strip()

    if s.split('$') != current: raise HTTPError(401, 'Wrong password')



class RebootHandler(bbctrl.APIHandler):
    def put_ok(self): subprocess.Popen('reboot')


class HostnameHandler(bbctrl.APIHandler):
    def get(self): self.write_json(socket.gethostname())

    def put(self):
        if 'hostname' in self.json:
            if subprocess.call(['/usr/local/bin/sethostname',
                                self.json['hostname'].strip()]) == 0:
                self.write_json('ok')
                return

        raise HTTPError(400, 'Failed to set hostname')


class UsernameHandler(bbctrl.APIHandler):
    def get(self): self.write_json(get_username())


    def put_ok(self):
        if 'username' in self.json: set_username(self.json['username'])
        else: raise HTTPError(400, 'Missing "username"')


class PasswordHandler(bbctrl.APIHandler):
    def put(self):
        if 'current' in self.json and 'password' in self.json:
            check_password(self.json['current'])

            # Set password
            s = '%s:%s' % (username, self.json['password'])
            s = s.encode('utf-8')

            p = subprocess.Popen(['chpasswd', '-c', 'MD5'],
                                 stdin = subprocess.PIPE)
            p.communicate(input = s)

            if p.returncode == 0:
                self.write_json('ok')
                return

        raise HTTPError(401, 'Failed to set password')


class ConfigLoadHandler(bbctrl.APIHandler):
    def get(self): self.write_json(self.ctrl.config.load())


class ConfigDownloadHandler(bbctrl.APIHandler):
    def set_default_headers(self):
        fmt = socket.gethostname() + '-%Y%m%d.json'
        filename = datetime.date.today().strftime(fmt)
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


    def put_ok(self):
        # Only allow this function in dev mode
        if not os.path.exists('/etc/bbctrl-dev-mode'):
            raise HTTPError(403, 'Not in dev mode')

        firmware = self.request.files['firmware'][0]

        if not os.path.exists('firmware'): os.mkdir('firmware')

        with open('firmware/update.tar.bz2', 'wb') as f:
            f.write(firmware['body'])

        subprocess.Popen(['/usr/local/bin/update-bbctrl'])


class UpgradeHandler(bbctrl.APIHandler):
    def put_ok(self):
        check_password(self.json['password'])
        subprocess.Popen(['/usr/local/bin/upgrade-bbctrl'])


class HomeHandler(bbctrl.APIHandler):
    def put_ok(self, axis, set_home):
        if axis is not None: axis = ord(axis[1:2].lower())

        if set_home:
            if not 'position' in self.json:
                raise HTTPError(400, 'Missing "position"')

            self.ctrl.avr.home(axis, self.json['position'])

        else: self.ctrl.avr.home(axis)


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


class PositionHandler(bbctrl.APIHandler):
    def put_ok(self, axis):
        self.ctrl.avr.set_position(ord(axis.lower()), self.json['position'])


class OverrideFeedHandler(bbctrl.APIHandler):
    def put_ok(self, value): self.ctrl.avr.override_feed(float(value))


class OverrideSpeedHandler(bbctrl.APIHandler):
    def put_ok(self, value): self.ctrl.avr.override_speed(float(value))


class JogHandler(bbctrl.APIHandler):
    def put_ok(self): self.ctrl.avr.jog(self.json)


# Base class for Web Socket connections
class ClientConnection(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.count = 0


    def heartbeat(self):
        self.ctrl.ioloop.call_later(3, self.heartbeat)
        self.send({'heartbeat': self.count})
        self.count += 1


    def notify(self, msg): self.send(dict(msg = msg))
    def send(self, msg): raise HTTPError(400, 'Not implemented')


    def on_open(self, *args, **kwargs):
        self.timer = self.ctrl.ioloop.call_later(3, self.heartbeat)
        self.ctrl.state.add_listener(self.send)
        self.ctrl.msgs.add_listener(self.notify)
        self.is_open = True


    def on_close(self):
        self.ctrl.ioloop.remove_timeout(self.timer)
        self.ctrl.state.remove_listener(self.send)
        self.ctrl.msgs.remove_listener(self.notify)


    def on_message(self, data): self.ctrl.avr.mdi(data)


# Used by CAMotics
class WSConnection(ClientConnection, tornado.websocket.WebSocketHandler):
    def __init__(self, app, request, **kwargs):
        ClientConnection.__init__(self, app.ctrl)
        tornado.websocket.WebSocketHandler.__init__(
            self, app, request, **kwargs)


    def send(self, msg): self.write_message(msg)
    def open(self): self.on_open()


# Used by Web frontend
class SockJSConnection(ClientConnection, sockjs.tornado.SockJSConnection):
    def __init__(self, session):
        ClientConnection.__init__(self, session.server.ctrl)
        sockjs.tornado.SockJSConnection.__init__(self, session)


    def send(self, msg): sockjs.tornado.SockJSConnection.send(self, msg)


class StaticFileHandler(tornado.web.StaticFileHandler):
    def set_extra_headers(self, path):
        self.set_header('Cache-Control',
                        'no-store, no-cache, must-revalidate, max-age=0')


class Web(tornado.web.Application):
    def __init__(self, ctrl):
        self.ctrl = ctrl

        handlers = [
            (r'/websocket', WSConnection),
            (r'/api/reboot', RebootHandler),
            (r'/api/hostname', HostnameHandler),
            (r'/api/remote/username', UsernameHandler),
            (r'/api/remote/password', PasswordHandler),
            (r'/api/config/load', ConfigLoadHandler),
            (r'/api/config/download', ConfigDownloadHandler),
            (r'/api/config/save', ConfigSaveHandler),
            (r'/api/config/reset', ConfigResetHandler),
            (r'/api/firmware/update', FirmwareUpdateHandler),
            (r'/api/upgrade', UpgradeHandler),
            (r'/api/file(/.+)?', bbctrl.FileHandler),
            (r'/api/home(/[xyzabcXYZABC](/set)?)?', HomeHandler),
            (r'/api/start(/.+)', StartHandler),
            (r'/api/estop', EStopHandler),
            (r'/api/clear', ClearHandler),
            (r'/api/stop', StopHandler),
            (r'/api/pause', PauseHandler),
            (r'/api/unpause', UnpauseHandler),
            (r'/api/pause/optional', OptionalPauseHandler),
            (r'/api/step(/.+)', StepHandler),
            (r'/api/position/([xyzabcXYZABC])', PositionHandler),
            (r'/api/override/feed/([\d.]+)', OverrideFeedHandler),
            (r'/api/override/speed/([\d.]+)', OverrideSpeedHandler),
            (r'/api/jog', JogHandler),
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
