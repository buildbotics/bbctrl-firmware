################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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
import uuid
from tornado.web import HTTPError
from . import util

__all__ = ['FileSystem']


class FileSystem:
  extensions = ('.nc', '.gc', '.gcode', '.ngc', '.tap', '.txt', '.tpl')
  protected = ('macros',)


  def __init__(self, ctrl):
    self.ctrl      = ctrl
    self.log       = ctrl.log.get('FS')

    upload = self.ctrl.root + '/upload'
    os.environ['GCODE_SCRIPT_PATH'] = upload

    if not os.path.exists(upload):
      os.mkdir(upload)
      from shutil import copy
      copy(util.get_resource('http/buildbotics.nc'), upload)

    self._ensure_macros_dir()
    self._update_locations()
    self._update_first_file()

    ctrl.events.on('invalidate', self._invalidate)
    ctrl.udevev.add_handler(self._udev_event, 'block')


  def _ensure_macros_dir(self):
    path = self.get_macros_dir()
    if not os.path.exists(path): os.makedirs(path, mode = 0o755)


  def is_protected_path(self, path):
    path = os.path.normpath(path).lstrip('./')

    for name in self.protected:
      if path == name or path == 'Home/' + name: return True
      if path.startswith(name + '/') or path.startswith('Home/' + name + '/'):
        return True

    return False


  def get_macros_dir(self):
    return self.ctrl.root + '/upload/macros'


  def isolate_macro(self, source):
    if (source.startswith('Home/macros/') or source.startswith('macros/')):
      return source

    real = self.realpath(source)
    if not real or not os.path.exists(real): return
    if not os.path.isfile(real):
      self.log.warning('Macro source is not a file: %s' % source)
      return

    base = os.path.basename(source)
    name = 'macro_%s_%s' % (uuid.uuid4().hex[:4], base)
    target = os.path.join(self.get_macros_dir(), name)

    try:
      shutil.copy2(real, target)
      self.log.info('Isolated macro: %s -> macros/%s' % (source, name))
      return 'macros/' + name

    except Exception as e:
      self.log.error('Failed to isolate macro: %s' % e)


  def cleanup_orphaned_macros(self, active):
    path = self.get_macros_dir()
    if not os.path.exists(path): return

    keep = set()
    for entry in active:
      if entry.startswith('macros/'): keep.add(os.path.basename(entry))
      elif entry.startswith('Home/macros/'):
        keep.add(os.path.basename(entry))

    for name in os.listdir(path):
      target = os.path.join(path, name)
      if not os.path.isfile(target): continue
      if not name.startswith('macro_'): continue
      if name in keep: continue

      try:
        os.unlink(target)
        self.log.info('Deleted orphaned macro %s' % name)
      except Exception as e:
        self.log.error('Failed to delete orphaned macro %s: %s' % (name, e))


  def _invalidate(self, path):
    if path == self.ctrl.state.get('first_file', ''):
      self._update_first_file()


  def _update_first_file(self):
    # Get GCode files from root upload directory
    upload = self.ctrl.root + '/upload'

    files = []
    for path in os.listdir(upload):
      if path in self.protected: continue
      parts = os.path.splitext(path)
      if (len(parts) == 2 and parts[1] in self.extensions and
          os.path.isfile(upload + '/' + path)):
        files.append(path)

    files.sort()

    # Set first file
    path = 'Home/' + files[0] if len(files) else ''
    self.ctrl.state.set('first_file', path)


  def validate_path(self, path):
    path = os.path.normpath(path)
    if path.startswith('..'): raise HTTPError(400, 'Invalid path')
    path = path.lstrip('./')

    realpath = self.realpath(path)
    if not os.path.exists(realpath): raise HTTPError(404, 'File not found')

    return path


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


  def delete(self, path):
    if self.is_protected_path(path):
      raise HTTPError(403, 'Cannot delete protected macro files')

    realpath = self.realpath(path)

    try:
      if os.path.isdir(realpath): shutil.rmtree(realpath, True)
      else: os.unlink(realpath)
    except OSError: pass

    self.log.info('Deleted ' + path)
    self.ctrl.events.emit('invalidate', path)


  def mkdir(self, path):
    if self.is_protected_path(path):
      raise HTTPError(403, 'Cannot create directories in protected area')

    realpath = self.realpath(path)

    if not os.path.exists(realpath):
      os.makedirs(realpath)
      os.sync()


  def write(self, path, data):
    if self.is_protected_path(path):
      raise HTTPError(403, 'Cannot write protected macro files')

    realpath = self.realpath(path)

    with open(realpath.encode('utf8'), 'wb') as f:
      f.write(data)

      self.log.info('Wrote ' + path)
      self.ctrl.events.emit('invalidate', path)
      os.sync()


  def _set_locations(self):
    self.ctrl.state.set('locations', list(self.locations.values()))


  def _update_locations(self):
    self.locations = {'home': 'Home'}

    with open('/proc/mounts', 'r') as f:
      for line in f:
        mount = line.split()

        if mount[1].startswith('/media/'):
          self.locations[mount[0]] = mount[1][7:]

    self._set_locations()


  def _udev_event(self, action, device):
    node = device.device_node

    if action == 'add' and device.get('ID_FS_USAGE', '') == 'filesystem':
      label = device.get('ID_FS_LABEL', '')
      if not label: label = 'USB_DISK-' + node.split('/')[-1]
      self.locations[node] = label
      self._set_locations()

    if action == 'remove' and node in self.locations:
      del self.locations[node]
      self._set_locations()
