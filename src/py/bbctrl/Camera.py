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
import socket
from tornado import gen, web, iostream

try:
    import v4l2
except:
    import bbctrl.v4l2 as v4l2

log = logging.getLogger('Camera')


offline_jpg = '''
/9j/4AAQSkZJRgABAQEASABIAAD/2wBDAAMCAgMCAgMDAwMEAwMEBQgFBQQEBQoHBwYIDAoMDAsKCwsN
DhIQDQ4RDgsLEBYQERMUFRUVDA8XGBYUGBIUFRT/2wBDAQMEBAUEBQkFBQkUDQsNFBQUFBQUFBQUFBQU
FBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBT/wgARCAHgAoADASEAAhEBAxEB/8QA
HAABAAIDAQEBAAAAAAAAAAAAAAYHBAUIAwIB/8QAFAEBAAAAAAAAAAAAAAAAAAAAAP/aAAwDAQACEAMQ
AAAB6pAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAR/XEizwAAAAAAAAAe
RF/ckv6IsRyySB68/bOIvtTZgAAAAAAAAA+OfTZSAhV3G6AAAAAAAAAarngn+GRnok8Oci5tYRWeFH9S
kL3huAAAAAAAAAAV1Ey8Q+CDV2edxkhqA+IwWnBTUXabqAFc5BdfwVjpiwY+RjJLuywKGnZPhS+5IzAy
XR4y7aKm6YK9kh4QYj3oXp7lYQk2F2GSAAAAAAKQl5YADAzzV0SdFcvF/bM5V6rI9XpbVFHRMfKnt055
6gPXAM+vTS24By50sZohFelvU8X5Uhvp6cz9MFLzkxauOiKiN3titL4IFGi4wAAAAABTO9LJAVBEjJ0J
1DzOdMDmfpg1FKlh0uST8M+1Spb5Pjn888Y3t5gcydCm2FfRAtWni/KkN9PTmfpgpecmLGS366MLxK73
J8yQuEAAAAAARqjTpH2NV7HOXTZp+czqXmg6XHNHS5qKWLgpQ6K/Ty0pU18GmpU6DrUit6AVRqi7PM5y
uM2dPF+VIb6enM/TBS85MWNFvV0YW5ILeY8j1AAAAAACAFZZJ+3oVpDTbi9KOLxFG3ka2sC4asIPkk6m
BXttHzQR5bM3VrgfFOEc+SwbKNVWxb1dG3lxRt5FYSsx9IWPCjGnpTccPeyScAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAP//EACwQAAEEAgED
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
hYlrhe5ZUSmBUvBrjSoubOGnD113nR+6Ll2xD5ZK0MXV8iKXWyIK8yqrmXxCVZnUPl77vMcAZEqyNWOW
EC7ceCUNAMpJd/7U4rPiEzdietSEkN+i7n4AwCWR1pM84MB7ZPej71oIE5MUilzMDqWfVXnEEubm7xwv
rT0JTAvWuoEeB1OE+whsTmFSXD6osuKYbhGAQTZlpgL9MGcxs1EDhEhc5ltal2AgSgrdYqCkpuQOJw5R
H8J1RUp1riwpudms6fOfJwHj/TRSwgyDrFM1JssRQ2K1NIcXPP8A4Pf/2gAMAwEAAgADAAAAEMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMxMxMzMzMzMzMzMwIjM
zMzMzMzMAECMwMzMzMzMzMzMzASICIRMBMwMTIzIzMAMzMzMzMTMSMSMjACMTExAzMQIxMzMzMzMwMxE
zMwEBMhMjMDMxEzEDMzMzMzITMSMQEwECMzMwMzETIzMzMzMzMgIiIzEzIQAzMjExMiMQAzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzM
zMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzP/EABQR
AQAAAAAAAAAAAAAAAAAAALD/2gAIAQMBAT8QFg//xAAUEQEAAAAAAAAAAAAAAAAAAACw/9oACAECAQE/
EBYP/8QAKRABAQABAwQCAwACAgMAAAAAAREhADFBEFFhcYGhIECRsfAwsMHR8f/aAAgBAQABPxD/AKCx
5PJZfYQPhDVWTbFf0D71Av1O+oVHwz981awh+6kA9ut/DT8aCfh0gfhUVwAafBoCRESiNE6oDkEQEpjF
SIIURCZ0j2FAFWDz+tIPMmAGVXYDeupROBsbw0eUjw9KdJkBY16cS9fwUluCXZsxSxoaIrADK2XH77QX
sgIqq4ABVdRSvwPYLMFYHcbSgMUMQDvDMJ2AedOyAx/JDfenFwYXG6CsDlOVAzouJDAqFQAVc7fu8aQF
dgLurjwVcDCtclgeFdsCCxVQDJpStAlU5Bah3hexoCYKMx4DAeCwu6GdOszTLjGuSbpwbRR0u+YqEIPI
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
qhJEhSpcEm7f+h7/AP/Z
'''


def array_to_string(a): return ''.join([chr(i) for i in a])


def fourcc_to_string(i):
    return \
        chr((i >>  0) & 0xff) + \
        chr((i >>  8) & 0xff) + \
        chr((i >> 16) & 0xff) + \
        chr((i >> 24) & 0xff)


def string_to_fourcc(s): return v4l2.v4l2_fourcc(s[0], s[1], s[2], s[3])


class VideoDevice(object):
    def __init__(self, path = '/dev/video0'):
        self.fd = os.open(path, os.O_RDWR | os.O_NONBLOCK | os.O_CLOEXEC)
        self.buffers = []


    def fileno(self): return self.fd


    def get_audio(self):
        b = v4l2.v4l2_audio()
        b.index = 0

        l = []

        while True:
            try:
                fcntl.ioctl(self, v4l2.VIDIOC_ENUMAUDIO, b)
                l.append((array_to_string(b.name), b.capability, b.mode))
                b.index += 1

            except OSError: break

        return l


    def get_formats(self):
        b = v4l2.v4l2_fmtdesc()
        b.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        b.index = 0

        l = []

        while True:
            try:
                fcntl.ioctl(self, v4l2.VIDIOC_ENUM_FMT, b)

                l.append((fourcc_to_string(b.pixelformat),
                          array_to_string(b.description)))

                b.index += 1

            except OSError: break

        return l


    def get_frame_sizes(self, fourcc):
        b = v4l2.v4l2_frmsizeenum()
        b.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        b.pixel_format = fourcc

        sizes = []

        while True:
            try:
                fcntl.ioctl(self, v4l2.VIDIOC_ENUM_FRAMESIZES, b)

                if b.type == v4l2.V4L2_FRMSIZE_TYPE_DISCRETE:
                    sizes.append((b.discrete.width, b.discrete.height))

                else:
                    sizes.append((b.stepwise.min_width, b.stepwise.max_width,
                                  b.stepwise.step_width, b.stepwise.min_height,
                                  b.stepwise.max_height,
                                  b.stepwise.step_height))

                b.index += 1

            except OSError: break

        return sizes


    def set_format(self, width, height, fourcc):
        fmt = v4l2.v4l2_format()
        fmt.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        fcntl.ioctl(self, v4l2.VIDIOC_G_FMT, fmt)

        fmt.fmt.pix.width = width
        fmt.fmt.pix.height = height
        fmt.fmt.pix.pixelformat = fourcc

        fcntl.ioctl(self, v4l2.VIDIOC_S_FMT, fmt)


    def create_buffers(self, count):
        # Create buffers
        rbuf = v4l2.v4l2_requestbuffers()
        rbuf.count = count;
        rbuf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE;
        rbuf.memory = v4l2.V4L2_MEMORY_MMAP;

        fcntl.ioctl(self, v4l2.VIDIOC_REQBUFS, rbuf)

        for i in range(rbuf.count):
            # Get buffer
            buf = v4l2.v4l2_buffer()
            buf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
            buf.memory = v4l2.V4L2_MEMORY_MMAP
            buf.index = i
            fcntl.ioctl(self, v4l2.VIDIOC_QUERYBUF, buf)

            # Mem map buffer
            mm = mmap.mmap(self.fileno(), buf.length, mmap.MAP_SHARED,
                           mmap.PROT_READ | mmap.PROT_WRITE,
                           offset = buf.m.offset)
            self.buffers.append(mm)

            # Queue the buffer for capture
            fcntl.ioctl(self, v4l2.VIDIOC_QBUF, buf)


    def _dqbuf(self):
        buf = v4l2.v4l2_buffer()
        buf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        buf.memory = v4l2.V4L2_MEMORY_MMAP
        fcntl.ioctl(self, v4l2.VIDIOC_DQBUF, buf)

        return buf


    def _qbuf(self, buf):
        fcntl.ioctl(self, v4l2.VIDIOC_QBUF, buf)


    def read_frame(self):
        buf = self._dqbuf()
        mm = self.buffers[buf.index]

        frame = mm.read()
        mm.seek(0)
        self._qbuf(buf)

        return frame


    def flush_frame(self): self._qbuf(self._dqbuf())


    def get_info(self):
        caps = v4l2.v4l2_capability()
        fcntl.ioctl(self, v4l2.VIDIOC_QUERYCAP, caps)

        caps._driver   = array_to_string(caps.driver)
        caps._card     = array_to_string(caps.card)
        caps._bus_info = array_to_string(caps.bus_info)

        l = []
        c = caps.capabilities
        if c & v4l2.V4L2_CAP_VIDEO_CAPTURE: l.append('video_capture')
        if c & v4l2.V4L2_CAP_VIDEO_OUTPUT: l.append('video_output')
        if c & v4l2.V4L2_CAP_VIDEO_OVERLAY: l.append('video_overlay')
        if c & v4l2.V4L2_CAP_VBI_CAPTURE: l.append('vbi_capture')
        if c & v4l2.V4L2_CAP_VBI_OUTPUT: l.append('vbi_output')
        if c & v4l2.V4L2_CAP_SLICED_VBI_CAPTURE: l.append('sliced_vbi_capture')
        if c & v4l2.V4L2_CAP_SLICED_VBI_OUTPUT: l.append('sliced_vbi_output')
        if c & v4l2.V4L2_CAP_RDS_CAPTURE: l.append('rds_capture')
        if c & v4l2.V4L2_CAP_VIDEO_OUTPUT_OVERLAY:
            l.append('video_output_overlay')
        if c & v4l2.V4L2_CAP_HW_FREQ_SEEK: l.append('hw_freq_seek')
        if c & v4l2.V4L2_CAP_RDS_OUTPUT: l.append('rds_output')
        if c & v4l2.V4L2_CAP_TUNER: l.append('tuner')
        if c & v4l2.V4L2_CAP_AUDIO: l.append('audio')
        if c & v4l2.V4L2_CAP_RADIO: l.append('radio')
        if c & v4l2.V4L2_CAP_MODULATOR: l.append('modulator')
        if c & v4l2.V4L2_CAP_READWRITE: l.append('readwrite')
        if c & v4l2.V4L2_CAP_ASYNCIO: l.append('asyncio')
        if c & v4l2.V4L2_CAP_STREAMING: l.append('streaming')
        caps._caps = l

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
        try:
            os.close(self.fd)
        except Exception as e: log.warning('While closing camera: %s', e)
        finally: self.fd = None


class Camera(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.width = ctrl.args.width
        self.height = ctrl.args.height
        self.fps = ctrl.args.fps
        self.fourcc = string_to_fourcc(ctrl.args.fourcc)

        self.offline_jpg = self._format_frame(base64.b64decode(offline_jpg))
        self.dev = None
        self.clients = []
        self.path = None

        # Find connected cameras
        for i in range(4):
            path = '/dev/video%d' % i
            if os.path.exists(path):
                self.open(path)
                break

        # Get notifications of camera (un)plug events
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


    def _format_frame(self, frame):
        frame = [
            b'Content-type: image/jpeg\r\n',
            b'Content-length: ', str(len(frame)).encode('utf8'), b'\r\n\r\n',
            frame, VideoHandler.boundary.encode('utf8'), b'\n']

        return b''.join(frame)


    def _send_frame(self, frame):
        frame = self._format_frame(frame)

        for client in self.clients:
            try:
                client.write_frame(frame)
            except Exception as e:
                log.warning('Failed to write frame to client: %s' % e)


    def _fd_handler(self, fd, events):
        try:
            if len(self.clients):
                frame = self.dev.read_frame()
                self._send_frame(frame)

            else: self.dev.flush_frame()

        except Exception as e:
            if isinstance(e, BlockingIOError): return

            log.warning('Failed to read from camera.')
            self.ctrl.ioloop.remove_handler(fd)
            self.close()
            return



    def open(self, path):
        try:
            self.path = path
            self.dev = VideoDevice(path)

            caps = self.dev.get_info()
            log.info('%s, %s, %s, %s', caps._driver, caps._card, caps._bus_info,
                     caps._caps)

            if caps.capabilities & v4l2.V4L2_CAP_VIDEO_CAPTURE == 0:
                raise Exception('Video capture not supported.')

            log.info('Formats: %s', self.dev.get_formats())
            log.info('Sizes: %s', self.dev.get_frame_sizes(self.fourcc))
            log.info('Audio: %s', self.dev.get_audio())

            self.dev.set_format(self.width, self.height, fourcc = self.fourcc)
            self.dev.set_fps(self.fps)
            self.dev.create_buffers(4)
            self.dev.start()

            self.ctrl.ioloop.add_handler(self.dev, self._fd_handler,
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
                client.write_frame(self.offline_jpg)
                client.write_frame(self.offline_jpg)

            log.info('Closed camera %s' % self.path)

        except: log.warning('Closing camera')
        finally: self.dev = None


    def add_client(self, client):
        log.info('Adding camera client: %d' % len(self.clients))
        self.clients.append(client)

        if self.dev is None:
            client.write_frame(self.offline_jpg)
            client.write_frame(self.offline_jpg)


    def remove_client(self, client):
        log.info('Removing camera client')
        try:
            self.clients.remove(client)
        except: pass



class VideoHandler(web.RequestHandler):
    boundary = '---boundary---'


    def __init__(self, app, request, **kwargs):
        super().__init__(app, request, **kwargs)
        self.camera = app.ctrl.camera


    @web.asynchronous
    def get(self):
        self.request.connection.stream.max_write_buffer_size = 10000

        self.set_header('Cache-Control', 'no-store, no-cache, ' +
                        'must-revalidate, pre-check=0, post-check=0, ' +
                        'max-age=0')
        self.set_header('Connection', 'close')
        self.set_header('Content-Type', 'multipart/x-mixed-replace;boundary=' +
                        self.boundary)
        self.set_header('Expires', 'Mon, 3 Jan 2000 12:34:56 GMT')
        self.set_header('Pragma', 'no-cache')

        self.camera.add_client(self)


    def write_frame(self, frame):
        # Don't allow too many frames to queue up
        min_size = len(frame) * 2
        if self.request.connection.stream.max_write_buffer_size < min_size:
            self.request.connection.stream.max_write_buffer_size = min_size

        try:
            self.write(frame)
            self.flush()

        except iostream.StreamBufferFullError:
            log.info('Camera buffer full')
            pass # Drop frame if buffer is full


    def on_connection_close(self): self.camera.remove_client(self)



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

    import argparse
    parser = argparse.ArgumentParser(description = 'Camera Server Test')
    parser.add_argument('--width', default = 640, type = int)
    parser.add_argument('--height', default = 480, type = int)
    parser.add_argument('--fps', default = 15, type = int)
    parser.add_argument('--fourcc', default = 'MJPG')
    args = parser.parse_args()


    logging.basicConfig(level = logging.INFO)

    ctrl = Ctrl()
    ctrl.args = args
    ctrl.camera = Camera(ctrl)
    server = Web(ctrl)
    ctrl.ioloop.start()
