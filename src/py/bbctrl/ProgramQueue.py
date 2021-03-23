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
import bbctrl


class ProgramQueue:
  def __init__(self, ctrl):
    self.ctrl = ctrl
    self.log = ctrl.log.get('Queue')

    self.q = []
    self.updating = False
    self.dirty = True

    ctrl.events.on('invalidate', self.invalidate)
    ctrl.events.on('invalidate-all', self.invalidate_all)

    self.next()
    self._update_progress()


  def _update_progress(self):
    path = self.get()

    if path:
      progress = self.ctrl.preplanner.get_plan_progress(path)
      self.ctrl.state.set('queued_progress', progress)

    self.ctrl.ioloop.call_later(1, self._update_progress)


  def invalidate(self, path):
    if path == self.get():
      self.ctrl.ioloop.add_callback(self.update)


  def invalidate_all(self):
    self.ctrl.ioloop.add_callback(self.update)


  def get(self): return self.ctrl.state.get('queued', '')


  def load(self, path):
    if self.get() == path: return;
    self.log.info('Loading ' + path)

    realpath = self.ctrl.fs.realpath(path)

    if os.path.exists(realpath): modified = os.path.getmtime(realpath)
    else: path, modified = '', 0

    state = self.ctrl.state
    state.set('queued', path)
    state.set('queued_modified', modified)
    state.set('queued_time', 0)
    state.set('queued_messages', [])
    state.set('line', 0)
    self.clear_bounds()
    self.update()


  def set(self, path):
    self.load(path)
    self.q = [path]


  def next(self):
    # Get GCode files from root upload directory
    upload = self.ctrl.root + '/upload'

    files = []
    for path in os.listdir(upload):
      parts = os.path.splitext(path)
      if (len(parts) == 2 and parts[1] in ('.nc', '.gc', '.gcode') and
          os.path.isfile(upload + '/' + path)):
        files.append(path)

    files.sort()

    # Select first file
    if len(files): self.set('Home/' + files[0])
    else: self.set('')


  def push(self, path):
    self.log.info('push(%s)' % path)

    self.q.append(path)
    self.load(path)


  def pop(self):
    self.log.info('pop() q=%s' % self.q)

    if 1 < len(self.q):
      self.q.pop()
      self.load(self.q[-1])


  def update(self):
    self.dirty = True
    if self.updating: return

    path = self.get()
    if not path: return

    def _update(future):
      if path == self.get():
        meta = future.result()[0]

        self.set_bounds(meta['bounds'])
        self.ctrl.state.set('queued_time', meta['time'])
        self.ctrl.state.set('queued_messages', meta['messages'])
        self.dirty = False

      self.updating = False
      if self.dirty: self.update()

    future = self.ctrl.preplanner.get_plan(path)
    self.ctrl.ioloop.add_future(future, _update)


  def set_bounds(self, bounds):
      for axis in 'xyzabc':
          for name in ('min', 'max'):
              value = bounds[name][axis] if axis in bounds[name] else 0
              self.ctrl.state.set('queued_%s_%s' % (name, axis), value)


  def clear_bounds(self):
      for axis in 'xyzabc':
          for name in ('min', 'max'):
              self.ctrl.state.set('queued_%s_%s' % (name, axis), 0)
