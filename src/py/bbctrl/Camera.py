#!/usr/bin/env python3
################################################################################
#                                                                              #
#                This file is part of the Buildbotics firmware.                #
#                                                                              #
#                  Copyright (c) 2015 - 2018, Buildbotics LLC                  #
#                             All rights reserved.                             #
#                                                                              #
#     This file ("the software") is free software: you can redistribute it     #
#     and/or modify it under the terms of the GNU General Public License,      #
#      version 2 as published by the Free Software Foundation. You should      #
#      have received a copy of the GNU General Public License, version 2       #
#     along with the software. If not, see <http://www.gnu.org/licenses/>.     #
#                                                                              #
#     The software is distributed in the hope that it will be useful, but      #
#          WITHOUT ANY WARRANTY; without even the implied warranty of          #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       #
#               Lesser General Public License for more details.                #
#                                                                              #
#       You should have received a copy of the GNU Lesser General Public       #
#                License along with the software.  If not, see                 #
#                       <http://www.gnu.org/licenses/>.                        #
#                                                                              #
#                For information regarding this software email:                #
#                  "Joseph Coffland" <joseph@buildbotics.com>                  #
#                                                                              #
################################################################################

import os
import fcntl
import logging
import select
import struct
import mmap
import pyudev
import base64
from tornado import gen, web

try:
    import v4l2
except:
    import bbctrl.v4l2 as v4l2

log = logging.getLogger('Camera')


offline_jpg = '''
/9j/4AAQSkZJRgABAQEASABIAAD/2wBDAAMCAgMCAgMDAwMEAwMEBQgFBQQEBQoHBwYIDAoMDAsKCwsN
DhIQDQ4RDgsLEBYQERMUFRUVDA8XGBYUGBIUFRT/2wBDAQMEBAUEBQkFBQkUDQsNFBQUFBQUFBQUFBQU
FBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBT/wgARCAHgAoADASEAAhEBAxEB/8QA
HQABAAMAAwEBAQAAAAAAAAAAAAYHCAMEBQkCAf/EABQBAQAAAAAAAAAAAAAAAAAAAAD/2gAMAwEAAhAD
EAAAAcqAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJl7hB/JAAAAAAAAA
BzFg9Qgn8Fgk6oYuP2z80CT+OHhgAAAAAAAAA/e1jwIYWtksiwAAAAAAAACQbeKX9UnuHju7nMryAsSn
TW/zzLUiJGQAAAAAAAAAXfY5kkP2W7eJzZbIZpo5LBM72+SXJpFrlL06JlDkNASkpWaFg+eZL88DYtPF
NjVMWJ9cRWk3PMzUaSwSXTBTt28TbgMhdU0BbJ4mTjpAAAAAADWlZFMAPX8g97YZh76DGMPAPov85ya3
aZs2AYdmppHM5tv58nB655F0ktzSB9CcEnmC2brMz6eMa6TIdTpvbBJqmoj0dCGH9MkSjZfmOy4p6ZfA
AAAAAGpoeUSA03ZB0ZifPnexgkb2wSSXVhSWqyCDx86mk8dH72kc3oEPyIBvvE5HBdNmmddPGNdJkOp0
3tgk1TUR6M8MzXger2i8IocsDMxAAAAAACd67MI9ckHVNz4CJPuU+ee9jBI3tgkkurDMOrzD38OaWGkc
eEo1cYpvcsTIgGjJEZR5DdGXDwtPGNdJkOp03tgk1TUR6M8MzXgerFi3sijmOEAAAAAALlL/AOgfnIJf
VqEYGQ9cGRxrjI57mgDMehS3OiVBWJdmbT9bMOz4BE85gfrUZOeUpOiCQ3sZou8jVamuMjl/1yd2WlGW
oehTZqScnUocqUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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
xeI9Jg+f7jZh7bBexx7LX6DbhkYLqCA+ayQvwO9SvCKmS8AWXJjO1CZIc4dfxpnZmy3pbdXtekqYRHBP
8ULiJeIHu/FICM5RdSDyKs9BKgpgp4li9Espl7pRMp8Lci/7vKnkovQxOF3d0aZ80t0WS7g1h1wyj5tk
hBmjXHOerGkSW064WFiIsoUF2xWUjdpEmTMxgWtu9AHI8nWwNGLrHdzXmmwKXQfKVbmkpeSByL9OKudb
FBVYLvatawr4k2sHbKkDCNZMTmHKW0E6llP0dDvtO0U0ph+MyiJjdielZFXWZEZXoRM6UQjTiYE4D3+G
e2tK7sgmbpm81JmS4eMvHkq4yo7e0PATO+mF1zlJaUF2xpWmWYyJMnqmGS8QiTi9WavokiNgOeGwBT+b
FMk1w/3j/wDUdTesjJlUA2A5jUXDVgwlguaVm9eakJlYCOoXywgrpWU3VhMiCchWVH6SkR8p4Su2tLLp
iJPIu9qdQNBkdJtqANOhkmjJbf8Age//2gAMAwEAAgADAAAAEMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMxMzMzMzMzMzMhMSEzMzMzMzMzMyMjMzMzMzMzMiMCEyMzMzMzMzMzM
zITMyMAMxExMBAxMjEzMzMzMzMjMSIjAAMCMTMwAzEAMSEzMzMzMxMwAzMiExEBMzADMQEgATMzMzMzE
xECMSITMDMzIwMxACATMzMzMzMxEhMzEyAwAzIBEhMAIRIzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzP/EABQRAQAAAAAAAAAAAAAAAAAAALD/
2gAIAQMBAT8QFg//xAAUEQEAAAAAAAAAAAAAAAAAAACw/9oACAECAQE/EBYP/8QAJxABAQABBAIBBAMB
AQEAAAAAAREhADFBURBhcSBAgaEwkfCxsNH/2gAIAQEAAT8Q/wDAsP8AcRp7cb2LpQBmH9ob+JqqmIB7
MAPafv8A0tsUcJV9BohH0Pyc9+TQYx7HpUAduNOQRFER8ujwZQMcqQ0UgiLcaKIGawFQ23Hejc5TLMAG
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
'''

offline_jpg = base64.b64decode(offline_jpg)


class VideoDevice(object):
    def __init__(self, path = '/dev/video0'):
        self.fd = os.open(path, os.O_RDWR | os.O_NONBLOCK | os.O_CLOEXEC)
        self.buffers = []


    def fileno(self): return self.fd


    def set_format(self, width = 640, height = 480,
                   fourcc = v4l2.V4L2_PIX_FMT_MJPEG):
        fmt = v4l2.v4l2_format()
        fmt.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        fcntl.ioctl(self, v4l2.VIDIOC_G_FMT, fmt)

        fmt.fmt.pix.width = width
        fmt.fmt.pix.height = height
        fmt.fmt.pix.pixelformat = fourcc

        fcntl.ioctl(self, v4l2.VIDIOC_S_FMT, fmt)


    def create_buffers(self, count):
        rbuf = v4l2.v4l2_requestbuffers()
        rbuf.count = count;
        rbuf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE;
        rbuf.memory = v4l2.V4L2_MEMORY_MMAP;

        fcntl.ioctl(self, v4l2.VIDIOC_REQBUFS, rbuf)

        for i in range(rbuf.count):
            buf = v4l2.v4l2_buffer()
            buf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
            buf.memory = v4l2.V4L2_MEMORY_MMAP
            buf.index = i
            fcntl.ioctl(self, v4l2.VIDIOC_QUERYBUF, buf)

            mm = mmap.mmap(self.fileno(), buf.length, mmap.MAP_SHARED,
                           mmap.PROT_READ | mmap.PROT_WRITE,
                           offset = buf.m.offset)
            self.buffers.append(mm)

            # Queue the buffer for capture
            fcntl.ioctl(self, v4l2.VIDIOC_QBUF, buf)


    def read(self):
        if not len(self.buffers):
            raise Exception('Buffers have not been created.')

        buf = v4l2.v4l2_buffer()
        buf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        buf.memory = v4l2.V4L2_MEMORY_MMAP
        fcntl.ioctl(self, v4l2.VIDIOC_DQBUF, buf)

        mm = self.buffers[buf.index]
        frame = mm.read()
        mm.seek(0)

        fcntl.ioctl(self, v4l2.VIDIOC_QBUF, buf)

        return frame


    def get_info(self):
        caps = v4l2.v4l2_capability()
        fcntl.ioctl(self, v4l2.VIDIOC_QUERYCAP, caps)

        caps._driver   = ''.join([chr(i) for i in caps.driver])
        caps._card     = ''.join([chr(i) for i in caps.card])
        caps._bus_info = ''.join([chr(i) for i in caps.bus_info])

        return caps


    def set_fps(self, fps):
        setfps = v4l2.v4l2_streamparm()
        setfps.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE;
        setfps.parm.capture.timeperframe.numerator = 1
        setfps.parm.capture.timeperframe.denominator = fps
        fcntl.ioctl(self, v4l2.VIDIOC_S_PARM, setfps)


    def start(self):
        buf_type = v4l2.v4l2_buf_type(v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE)
        fcntl.ioctl(self, v4l2.VIDIOC_STREAMON, buf_type)


    def stop(self):
        buf_type = v4l2.v4l2_buf_type(v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE)
        fcntl.ioctl(self, v4l2.VIDIOC_STREAMOFF, buf_type)


    def close(self):
        if self.fd is None: return
        os.close(self.fd)
        self.fd = None


class Camera(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.dev = None
        self.clients = []
        self.path = None
        self.frames = 0

        for i in range(4):
            path = '/dev/video%d' % i
            if os.path.exists(path):
                self.open(path)
                break

        self.udevCtx = pyudev.Context()
        self.udevMon = pyudev.Monitor.from_netlink(self.udevCtx)
        self.udevMon.filter_by(subsystem = 'video4linux')
        ctrl.ioloop.add_handler(self.udevMon, self._udev_handler,
                                ctrl.ioloop.READ)
        self.udevMon.start()


    def _udev_handler(self, fd, events):
        action, device = self.udevMon.receive_device()
        if device is None or self.dev is not None: return

        path = str(device.device_node)

        if action == 'add': self.open(path)
        if action == 'remove' and path == self.path: self.close()


    def open(self, path):
        self.path = path

        try:
            self.dev = VideoDevice(path)

            caps = self.dev.get_info()
            log.info('%s, %s, %s', caps._driver, caps._card, caps._bus_info)

            if caps.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE == 0:
                raise Exception('Video capture not supported.')

            self.dev.set_format(640, 480, fourcc = v4l2.V4L2_PIX_FMT_MJPEG)
            self.dev.set_fps(15)
            self.dev.create_buffers(30)
            self.dev.start()

            self.ctrl.ioloop.add_handler(self.dev, self._handler,
                                         self.ctrl.ioloop.READ)

            log.info('Opened camera ' + path)


        except Exception as e:
            log.warning('While loading camera')
            if not self.dev is None:
                self.dev.close()
                self.dev = None


    def close(self):
        if self.dev is None: return
        try:
            self.ctrl.ioloop.remove_handler(self.dev)
            try:
                self.dev.stop()
            except: pass
            self.dev.close()

            for client in self.clients:
                client.write_frame(offline_jpg)
                client.write_frame(offline_jpg)

            log.info('Closed camera %s' % self.path)

        except: log.warning('Closing camera')
        finally: self.dev = None


    def _handler(self, fd, events):
        try:
            frame = self.dev.read()

        except:
            log.warning('Failed to read from camera.')
            self.ctrl.ioloop.remove_handler(fd)
            self.close()
            return

        if frame is not None:
            if self.frames < 10:
                with open('frame%d.jpg' % self.frames, 'wb') as f:
                    f.write(frame)

            self.frames += 1

            for client in self.clients:
                client.write_frame(frame)


    def add_client(self, client):
        log.info('Adding camera client: %d' % len(self.clients))
        self.clients.append(client)
        if self.dev is None:
            client.write_frame(offline_jpg)
            client.write_frame(offline_jpg)


    def remove_client(self, client):
        log.info('Removing camera client')
        try:
            self.clients.remove(client)
        except: pass



class VideoHandler(web.RequestHandler):
    def __init__(self, app, request, **kwargs):
        super().__init__(app, request, **kwargs)
        self.camera = app.ctrl.camera
        self.boundary = '---boundary---'


    @web.asynchronous
    def get(self):
        self.set_header('Cache-Control', 'no-store, no-cache, ' +
                        'must-revalidate, pre-check=0, post-check=0, ' +
                        'max-age=0')
        self.set_header('Connection', 'close')
        self.set_header('Content-Type',
                        'multipart/x-mixed-replace;boundary=' +
                        self.boundary)
        self.set_header('Expires', 'Mon, 3 Jan 2000 12:34:56 GMT')
        self.set_header('Pragma', 'no-cache')

        self.camera.add_client(self)


    def write_frame(self, frame):
        self.write("Content-type: image/jpeg\r\n")
        self.write("Content-length: %s\r\n\r\n" % len(frame))
        self.write(frame)
        self.write(self.boundary + '\n')
        self.flush()


    def on_connection_close(self):
        self.camera.remove_client(self)



if __name__ == '__main__':
    class Ctrl(object):
        def __init__(self):
            from tornado import ioloop
            self.ioloop = ioloop.IOLoop.current()


    class RootHandler(web.RequestHandler):
        def get(self):
            self.set_header('Content-Type', 'text/html')
            self.write('<html><body><img src="video"/></body></html>')


    class Web(web.Application):
        def __init__(self, ctrl):
            self.ctrl = ctrl

            handlers = [
                (r'/', RootHandler),
                (r'/video', VideoHandler)
            ]

            web.Application.__init__(self, handlers)
            self.listen(9000, address = '127.0.0.1')



    root = logging.getLogger()
    root.setLevel(logging.INFO)

    ctrl = Ctrl()
    ctrl.camera = Camera(ctrl)
    server = Web(ctrl)
    ctrl.ioloop.start()
