################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
#                                                                              #
################################################################################

import os
import sys
import json
import tornado
import sockjs.tornado
import datetime
import shutil
import tarfile
import subprocess
import socket
import time
from tornado.web import HTTPError
from tornado import web, gen
from tornado.concurrent import run_on_executor
from concurrent.futures import ThreadPoolExecutor

from . import util
from .Log import *
from .APIHandler import *
from .RequestHandler import *
from .Camera import *
from .MonitorTemp import *
from .FileSystemHandler import *
from .Ctrl import *

__all__ = ['Web']


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
    if len(current) < 2 or current[1] != '1':
        raise HTTPError(401, "Password invalid")

    # Check current password
    cmd = ['openssl', 'passwd', '-salt', current[2], '-1', password]
    s = call_get_output(cmd).strip()

    if s.split('$') != current: raise HTTPError(401, 'Wrong password')



class RebootHandler(APIHandler):
    def put_ok(self):
        self.get_ctrl().lcd.goodbye('Rebooting...')
        subprocess.Popen('reboot')


class StateHandler(APIHandler):
    def get(self, path):
        if path is None or path == '' or path == '/':
            self.write_json(self.get_ctrl().state.snapshot())
        else: self.write_json(self.get_ctrl().state.get(path[1:]))


class LogHandler(RequestHandler):
    def get(self):
        with open(self.get_ctrl().log.get_path(), 'r') as f:
            self.write(f.read())


    def set_default_headers(self):
        fmt = socket.gethostname() + '-%Y%m%d.log'
        filename = datetime.date.today().strftime(fmt)
        self.set_header('Content-Disposition', 'filename="%s"' % filename)
        self.set_header('Content-Type', 'text/plain')


class MessageAckHandler(APIHandler):
    def put_ok(self, id):
        self.get_ctrl().state.ack_message(int(id))


class BugReportHandler(RequestHandler):
    executor = ThreadPoolExecutor(max_workers = 4)


    def get_files(self):
        files = []

        def check_add(path, arcname = None):
            if os.path.isfile(path):
                if arcname is None: arcname = path
                files.append((path, self.basename + '/' + arcname))

        def check_add_basename(path):
            check_add(path, os.path.basename(path))

        ctrl = self.get_ctrl()
        path = ctrl.log.get_path()
        check_add_basename(path)
        for i in range(1, 8):
            check_add_basename('%s.%d' % (path, i))
        check_add_basename('/var/log/syslog')
        check_add(ctrl.config.get_path())
        # TODO Add recently run programs

        return files


    @run_on_executor
    def task(self):
        import tarfile, io

        files = self.get_files()

        buf = io.BytesIO()
        tar = tarfile.open(mode = 'w:bz2', fileobj = buf)
        for path, name in files: tar.add(path, name)
        tar.close()

        return buf.getvalue()


    @gen.coroutine
    def get(self):
        res = yield self.task()
        self.write(res)


    def set_default_headers(self):
        fmt = socket.gethostname() + '-%Y%m%d-%H%M%S'
        self.basename = datetime.datetime.now().strftime(fmt)
        filename = self.basename + '.tar.bz2'
        self.set_header('Content-Disposition', 'filename="%s"' % filename)
        self.set_header('Content-Type', 'application/x-bzip2')


class HostnameHandler(APIHandler):
    def get(self): self.write_json(socket.gethostname())

    def put(self):
        if self.get_ctrl().args.demo:
            raise HTTPError(400, 'Cannot set hostname in demo mode')

        if 'hostname' in self.json:
            if subprocess.call(['/usr/local/bin/sethostname',
                                self.json['hostname'].strip()]) == 0:
                self.write_json('ok')
                return

        raise HTTPError(400, 'Failed to set hostname')


class WifiHandler(APIHandler):
    def get(self):
        data = {'ssid': '', 'channel': 0}
        try:
            data = json.loads(call_get_output(['config-wifi', '-j']))
        except: pass
        self.write_json(data)


    def put(self):
        if self.get_ctrl().args.demo:
            raise HTTPError(400, 'Cannot configure WiFi in demo mode')

        if 'mode' in self.json:
            cmd = ['config-wifi', '-r']
            mode = self.json['mode']

            if mode == 'disabled': cmd += ['-d']
            elif 'ssid' in self.json:
                cmd += ['-s', self.json['ssid']]

                if 'internal' in self.json and not self.json['internal']:
                    cmd += ['-x']

                if mode == 'ap':
                    cmd += ['-a']
                    if 'channel' in self.json:
                        cmd += ['-c', self.json['channel']]

                if 'pass' in self.json:
                    cmd += ['-p', self.json['pass']]

            if subprocess.call(cmd) == 0:
                self.write_json('ok')
                return

        raise HTTPError(400, 'Failed to configure wifi')


class UsernameHandler(APIHandler):
    def get(self): self.write_json(get_username())


    def put_ok(self):
        if self.get_ctrl().args.demo:
            raise HTTPError(400, 'Cannot set username in demo mode')

        if 'username' in self.json: set_username(self.json['username'])
        else: raise HTTPError(400, 'Missing "username"')


class PasswordHandler(APIHandler):
    def put(self):
        if self.get_ctrl().args.demo:
            raise HTTPError(400, 'Cannot set password in demo mode')

        if 'current' in self.json and 'password' in self.json:
            check_password(self.json['current'])

            # Set password
            s = '%s:%s' % (get_username(), self.json['password'])
            s = s.encode('utf-8')

            p = subprocess.Popen(['chpasswd', '-c', 'MD5'],
                                 stdin = subprocess.PIPE)
            p.communicate(input = s)

            if p.returncode == 0:
                self.write_json('ok')
                return

        raise HTTPError(401, 'Failed to set password')


class ConfigLoadHandler(APIHandler):
    def get(self): self.write_json(self.get_ctrl().config.load())


class ConfigDownloadHandler(APIHandler):
    def set_default_headers(self):
        fmt = socket.gethostname() + '-%Y%m%d.json'
        filename = datetime.date.today().strftime(fmt)
        self.set_header('Content-Type', 'application/octet-stream')
        self.set_header('Content-Disposition',
                        'attachment; filename="%s"' % filename)

    def get(self):
        self.write_json(self.get_ctrl().config.load(), pretty = True)


class ConfigSaveHandler(APIHandler):
    def put_ok(self): self.get_ctrl().config.save(self.json)


class ConfigResetHandler(APIHandler):
    def put_ok(self): self.get_ctrl().config.reset()


class FirmwareUpdateHandler(APIHandler):
    def prepare(self): pass


    def put_ok(self):
        if not 'password' in self.request.arguments:
            raise HTTPError(401, 'Missing "password"')

        if not 'firmware' in self.request.files:
            raise HTTPError(401, 'Missing "firmware"')

        check_password(self.request.arguments['password'][0])

        firmware = self.request.files['firmware'][0]

        if not os.path.exists('firmware'): os.mkdir('firmware')

        with open('firmware/update.tar.bz2', 'wb') as f:
            f.write(firmware['body'])

        self.get_ctrl().lcd.goodbye('Upgrading firmware')
        subprocess.Popen(['/usr/local/bin/update-bbctrl'])


class UpgradeHandler(APIHandler):
    def put_ok(self):
        check_password(self.json['password'])
        self.get_ctrl().lcd.goodbye('Upgrading firmware')
        subprocess.Popen(['/usr/local/bin/upgrade-bbctrl'])


class USBUpdateHandler(APIHandler):
    def put_ok(self): self.get_ctrl().fs.usb_update()


class USBEjectHandler(APIHandler):
    def put_ok(self, path):
        subprocess.Popen(['/usr/local/bin/eject-usb', '/media/' + path])


class MacroHandler(APIHandler):
    def put_ok(self, macro):
        macros = self.get_ctrl().config.get('macros')

        macro = int(macro)
        if macro < 0 or len(macros) < macro:
            raise HTTPError(404, 'Invalid macro id %d' % macro)

        path = 'Home/' + macros[macro - 1]['path']

        if not self.get_ctrl().fs.exists(path):
            raise HTTPError(404, 'Macro file not found')

        self.get_ctrl().mach.start(path)


class PathHandler(APIHandler):
    @gen.coroutine
    def get(self, dataType, path, *args):
        if not os.path.exists(self.get_ctrl().fs.realpath(path)):
            raise HTTPError(404, 'File not found')

        preplanner = self.get_ctrl().preplanner
        future = preplanner.get_plan(path)

        try:
            delta = datetime.timedelta(seconds = 1)
            data = yield gen.with_timeout(delta, future)

        except gen.TimeoutError:
            progress = preplanner.get_plan_progress(path)
            self.write_json(dict(progress = progress))
            return

        try:
            if data is None: return
            meta, positions, speeds = data

            if dataType == 'positions': data = positions
            elif dataType == 'speeds': data = speeds
            else:
                self.write_json(meta)
                return

            filename = os.path.basename(path) + '-' + dataType + '.gz'
            self.set_header('Content-Disposition', 'filename="%s"' % filename)
            self.set_header('Content-Type', 'application/octet-stream')
            self.set_header('Content-Encoding', 'gzip')
            self.set_header('Content-Length', str(len(data)))

            # Respond with chunks to avoid long delays
            SIZE = 102400
            chunks = [data[i:i + SIZE] for i in range(0, len(data), SIZE)]
            for chunk in chunks:
                self.write(chunk)
                yield self.flush()

        except tornado.iostream.StreamClosedError as e: pass


class HomeHandler(APIHandler):
    def put_ok(self, axis, action, *args):
        if axis is not None: axis = ord(axis[1:2].lower())

        if action == '/set':
            if not 'position' in self.json:
                raise HTTPError(400, 'Missing "position"')

            self.get_ctrl().mach.home(axis, self.json['position'])

        elif action == '/clear': self.get_ctrl().mach.unhome(axis)
        else: self.get_ctrl().mach.home(axis)


class StartHandler(APIHandler):
    def put_ok(self, path):
        path = self.get_ctrl().fs.validate_path(path)
        self.get_ctrl().mach.start(path)


class ActivateHandler(APIHandler):
    def put_ok(self, path):
        path = self.get_ctrl().fs.validate_path(path)
        self.get_ctrl().state.set('active_program', path)


class EStopHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.estop()


class ClearHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.clear()


class StopHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.stop()


class PauseHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.pause()


class UnpauseHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.unpause()


class OptionalPauseHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.optional_pause()


class StepHandler(APIHandler):
    def put_ok(self): self.get_ctrl().mach.step()


class PositionHandler(APIHandler):
    def put_ok(self, axis):
        self.get_ctrl().mach.set_position(axis, float(self.json['position']))


class OverrideFeedHandler(APIHandler):
    def put_ok(self, value): self.get_ctrl().mach.override_feed(float(value))


class OverrideSpeedHandler(APIHandler):
    def put_ok(self, value): self.get_ctrl().mach.override_speed(float(value))


class ModbusReadHandler(APIHandler):
    def put_ok(self):
        self.get_ctrl().mach.modbus_read(int(self.json['address']))


class ModbusWriteHandler(APIHandler):
    def put_ok(self):
        self.get_ctrl().mach.modbus_write(int(self.json['address']),
                                    int(self.json['value']))


class JogHandler(APIHandler):
    def put_ok(self):
        # Handle possible out of order jog command processing
        if 'ts' in self.json:
            ts = self.json['ts']
            id = self.get_cookie('bbctrl-client-id')

            if not hasattr(self.app, 'last_jog'):
                self.app.last_jog = {}

            last = self.app.last_jog.get(id, 0)
            self.app.last_jog[id] = ts

            if ts < last: return # Out of order

        self.get_ctrl().mach.jog(self.json)


class KeyboardHandler(APIHandler):
    def set_keyboard(self, show):
        signal = 'SIGUSR' + ('1' if show else '2')
        subprocess.call(['killall', '-' + signal, 'bbkbd'])


    def put_ok(self, cmd, *args):
        show = cmd == 'show'
        enabled = self.get_ctrl().config.get('virtual-keyboard-enabled', True)
        if enabled or not show: self.set_keyboard(show)


# Base class for Web Socket connections
class ClientConnection(object):
    def __init__(self, app):
        self.app = app
        self.count = 0


    def heartbeat(self):
        self.timer = self.app.ioloop.call_later(3, self.heartbeat)
        self.send({'heartbeat': self.count})
        self.count += 1


    def send(self, msg): raise HTTPError(400, 'Not implemented')


    def on_open(self, id = None):
        self.ctrl = self.app.get_ctrl(id)

        self.ctrl.state.add_listener(self.send)
        self.ctrl.log.add_listener(self.send)
        self.is_open = True
        self.heartbeat()
        self.app.opened(self.ctrl)


    def on_close(self):
        self.app.ioloop.remove_timeout(self.timer)
        self.ctrl.state.remove_listener(self.send)
        self.ctrl.log.remove_listener(self.send)
        self.is_open = False
        self.app.closed(self.ctrl)


    def on_message(self, data): self.ctrl.mach.mdi(data)


# Used by CAMotics
class WSConnection(ClientConnection, tornado.websocket.WebSocketHandler):
    def __init__(self, app, request, **kwargs):
        ClientConnection.__init__(self, app)
        tornado.websocket.WebSocketHandler.__init__(
            self, app, request, **kwargs)

    def send(self, msg): self.write_message(msg)
    def open(self): self.on_open()


# Used by Web frontend
class SockJSConnection(ClientConnection, sockjs.tornado.SockJSConnection):
    def __init__(self, session):
        ClientConnection.__init__(self, session.server.app)
        sockjs.tornado.SockJSConnection.__init__(self, session)


    def send(self, msg):
        try:
            sockjs.tornado.SockJSConnection.send(self, msg)
        except:
            self.close()


    def on_open(self, info):
        cookie = info.get_cookie('bbctrl-client-id')
        if cookie is None: self.send(dict(sid = '')) # Trigger client reset
        else:
            id = cookie.value

            ip = info.ip
            if 'X-Real-IP' in info.headers: ip = info.headers['X-Real-IP']
            self.app.get_ctrl(id).log.get('Web').info('Connection from %s' % ip)
            super().on_open(id)


class StaticFileHandler(tornado.web.StaticFileHandler):
    def set_extra_headers(self, path):
        self.set_header('Cache-Control',
                        'no-store, no-cache, must-revalidate, max-age=0')


class Web(tornado.web.Application):
    def __init__(self, args, ioloop):
        self.args = args
        self.ioloop = ioloop
        self.ctrls = {}

        # Init camera
        if not args.disable_camera:
            if self.args.demo: log = Log(args, ioloop, 'camera.log')
            else: log = self.get_ctrl().log
            self.camera = Camera(ioloop, args, log.get('Camera'))
        else: self.camera = None

        # Init controller
        if not self.args.demo:
            self.get_ctrl()
            self.monitor = MonitorTemp(self)

        handlers = [
            (r'/websocket',                     WSConnection),
            (r'/api/state(/.*)?',               StateHandler),
            (r'/api/log',                       LogHandler),
            (r'/api/message/(\d+)/ack',         MessageAckHandler),
            (r'/api/bugreport',                 BugReportHandler),
            (r'/api/reboot',                    RebootHandler),
            (r'/api/hostname',                  HostnameHandler),
            (r'/api/wifi',                      WifiHandler),
            (r'/api/remote/username',           UsernameHandler),
            (r'/api/remote/password',           PasswordHandler),
            (r'/api/config/load',               ConfigLoadHandler),
            (r'/api/config/download',           ConfigDownloadHandler),
            (r'/api/config/save',               ConfigSaveHandler),
            (r'/api/config/reset',              ConfigResetHandler),
            (r'/api/firmware/update',           FirmwareUpdateHandler),
            (r'/api/upgrade',                   UpgradeHandler),
            (r'/api/usb/update',                USBUpdateHandler),
            (r'/api/usb/eject/(.*)',            USBEjectHandler),
            (r'/api/fs/(.*)',                   FileSystemHandler),
            (r'/api/macro/(\d+)',               MacroHandler),
            (r'/api/(path)/(.*)',               PathHandler),
            (r'/api/(positions)/(.*)',          PathHandler),
            (r'/api/(speeds)/(.*)',             PathHandler),
            (r'/api/home(/[xyzabcXYZABC]((/set)|(/clear))?)?', HomeHandler),
            (r'/api/start/(.*)',                StartHandler),
            (r'/api/activate/(.*)',             ActivateHandler),
            (r'/api/estop',                     EStopHandler),
            (r'/api/clear',                     ClearHandler),
            (r'/api/stop',                      StopHandler),
            (r'/api/pause',                     PauseHandler),
            (r'/api/unpause',                   UnpauseHandler),
            (r'/api/pause/optional',            OptionalPauseHandler),
            (r'/api/step',                      StepHandler),
            (r'/api/position/([xyzabcXYZABC])', PositionHandler),
            (r'/api/override/feed/([\d.]+)',    OverrideFeedHandler),
            (r'/api/override/speed/([\d.]+)',   OverrideSpeedHandler),
            (r'/api/modbus/read',               ModbusReadHandler),
            (r'/api/modbus/write',              ModbusWriteHandler),
            (r'/api/jog',                       JogHandler),
            (r'/api/video',                     VideoHandler),
            (r'/api/keyboard/((show)|(hide))',  KeyboardHandler),
            (r'/(.*)',                          StaticFileHandler, {
                'path': util.get_resource('http/'),
                'default_filename': 'index.html'
            }),
        ]

        router = sockjs.tornado.SockJSRouter(SockJSConnection, '/sockjs')
        router.app = self

        tornado.web.Application.__init__(self, router.urls + handlers)

        try:
            self.listen(args.port, address = args.addr)

        except Exception as e:
            raise Exception('Failed to bind %s:%d: %s' % (
                args.addr, args.port, e))

        print('Listening on http://%s:%d/' % (args.addr, args.port))


    def get_image_resource(self, name):
        return util.get_resource('http/images/%s.jpg' % name)


    def opened(self, ctrl): ctrl.clear_timeout()


    def closed(self, ctrl):
        # Time out clients in demo mode
        if self.args.demo: ctrl.set_timeout(self._reap_ctrl, ctrl)


    def _reap_ctrl(self, ctrl):
        ctrl.close()
        del self.ctrls[ctrl.id]


    def get_ctrl(self, id = None):
        if not id or not self.args.demo: id = ''

        if not id in self.ctrls:
            ctrl = Ctrl(self.args, self.ioloop, id)
            self.ctrls[id] = ctrl

        else: ctrl = self.ctrls[id]

        return ctrl


    # Override default logger
    def log_request(self, handler):
        ctrl = self.get_ctrl(handler.get_cookie('bbctrl-client-id'))
        log = ctrl.log.get('Web')
        log.info("%d %s", handler.get_status(), handler._request_summary())
