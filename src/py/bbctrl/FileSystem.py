################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
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
import shutil
import bbctrl


class FileSystem:
  def __init__(self, ctrl):
    self.ctrl = ctrl
    self.log = ctrl.log.get('FileSystem')
    self.locations = ['Home']

    upload = self.ctrl.root + '/upload'
    os.environ['GCODE_SCRIPT_PATH'] = upload

    if not os.path.exists(upload):
      os.mkdir(upload)
      from shutil import copy
      copy(bbctrl.get_resource('http/buildbotics.nc'), upload)

    ctrl.events.on('invalidate', self.invalidate)
    ctrl.events.on('invalidate-all', self.invalidate_all)
    self.usb_update()
    self.queue_next_file()


  def queue_next_file(self):
    upload = self.ctrl.root + '/upload'

    files = []
    for path in os.listdir(upload):
      if os.path.isfile(upload + '/' + path):
        files.append(path)

    files.sort()

    if len(files): self.queue_file('Home/' + files[0])
    else: self.queue_file('')


  def realpath(self, path):
    path = os.path.normpath(path)
    parts = path.split('/', 1)

    if not len(parts): return ''
    path = parts[1] if len(parts) == 2 else ''

    if parts[0] == 'Home': return self.ctrl.root + '/upload/' + path

    usb = '/media/' + parts[0]
    if os.path.exists(usb): return usb + '/' + path

    return ''


  def exists(self, path): return os.path.exists(self.realpath(path))
  def isfile(self, path): return os.path.isfile(self.realpath(path))


  def invalidate(self, path):
    if path == self.ctrl.state.get('queued', ''):
      self.ctrl.ioloop.add_callback(self.requeue)


  def invalidate_all(self):
    self.ctrl.ioloop.add_callback(self.requeue)


  def requeue(self):
    self.queue_file(self.ctrl.state.get('queued', ''))


  def set_bounds(self, bounds):
      import json
      self.log.info('bounds %s' % json.dumps(bounds))

      for axis in 'xyzabc':
          for name in ('min', 'max'):
              value = bounds[name][axis] if axis in bounds[name] else 0
              self.ctrl.state.set('queued_%s_%s' % (name, axis), value)


  def clear_bounds(self):
      for axis in 'xyzabc':
          for name in ('min', 'max'):
              self.ctrl.state.set('queued_%s_%s' % (name, axis), 0)


  def queue_file(self, path):
    realpath = self.realpath(path)

    if os.path.exists(realpath): modified = os.path.getmtime(realpath)
    else: path, modified = '', 0

    state = self.ctrl.state
    state.set('queued', path)
    state.set('queued_modified', modified)
    state.set('queued_time', 0)
    state.set('queued_messages', [])
    state.set('line', 0)
    self.clear_bounds()

    if not modified: return


    def check_progress():
      if state.get('queued', '') != path: return
      progress = self.ctrl.preplanner.get_plan_progress(path)
      state.set('queued_progress', progress)
      if progress < 1: self.ctrl.ioloop.call_later(1, check_progress)

    check_progress()


    def set_state(future):
      meta = future.result()[0]

      self.set_bounds(meta['bounds'])
      state.set('queued_time', meta['time'])
      state.set('queued_messages', meta['messages'])

    future = self.ctrl.preplanner.get_plan(path)
    self.ctrl.ioloop.add_future(future, set_state)


  def delete(self, path):
    realpath = self.realpath(path)

    try:
      if os.path.isdir(realpath): shutil.rmtree(realpath, True)
      else: os.unlink(realpath)
    except OSError: pass

    self.log.info('Deleted ' + path)
    self.ctrl.events.emit('invalidate', path)


  def mkdir(self, path):
    realpath = self.realpath(path)

    if not os.path.exists(realpath):
      os.makedirs(realpath)
      os.sync()


  def write(self, path, data):
    realpath = self.realpath(path)

    with open(realpath.encode('utf8'), 'wb') as f:
      f.write(data)

      self.log.info('Wrote ' + path)
      self.ctrl.events.emit('invalidate', path)
      os.sync()


  def usb_update(self):
    self.locations = ['Home']

    for name in os.listdir('/media'):
      if os.path.isdir('/media/' + name):
        self.locations.append(name)

    self.ctrl.state.set('locations', self.locations)
