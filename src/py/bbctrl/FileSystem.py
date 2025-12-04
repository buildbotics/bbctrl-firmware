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
  
  # Protected directories that users cannot modify directly
  PROTECTED_DIRS = ['macros']


  def __init__(self, ctrl):
    self.ctrl      = ctrl
    self.log       = ctrl.log.get('FS')

    upload = self.ctrl.root + '/upload'
    os.environ['GCODE_SCRIPT_PATH'] = upload

    if not os.path.exists(upload):
      os.mkdir(upload)
      from shutil import copy
      copy(util.get_resource('http/buildbotics.nc'), upload)

    # Create protected macros directory
    self._ensure_macros_dir()

    self._update_locations()
    self._update_first_file()

    ctrl.events.on('invalidate', self._invalidate)
    ctrl.udevev.add_handler(self._udev_event, 'block')


  def _ensure_macros_dir(self):
    """Create the protected macros directory if it doesn't exist"""
    macros_dir = self.ctrl.root + '/upload/macros'
    if not os.path.exists(macros_dir):
      os.makedirs(macros_dir, mode=0o755)
      self.log.info('Created protected macros directory')


  def is_protected_path(self, path):
    """Check if a path is in a protected directory"""
    # Normalize the path
    path = os.path.normpath(path).lstrip('./')
    
    # Check if path starts with any protected directory
    for protected in self.PROTECTED_DIRS:
      # Check for Home/macros or just macros
      if path.startswith('Home/' + protected) or path.startswith(protected + '/'):
        return True
      if path == 'Home/' + protected or path == protected:
        return True
    
    return False


  def get_macros_dir(self):
    """Get the real path to the macros directory"""
    return self.ctrl.root + '/upload/macros'


  def isolate_macro(self, source_path):
    """
    Copy a macro file to the protected macros directory.
    
    Args:
      source_path: Original path to the macro file (relative, e.g., 'Home/file.nc')
      
    Returns:
      New isolated path (relative), or None if source doesn't exist
    """
    # Skip if already in macros directory
    if source_path.startswith('Home/macros/') or source_path.startswith('macros/'):
      return source_path
    
    # Get the real source path
    real_source = self.realpath(source_path)
    if not real_source or not os.path.exists(real_source):
      self.log.warning('Macro source not found: %s' % source_path)
      return None
    
    if not os.path.isfile(real_source):
      self.log.warning('Macro source is not a file: %s' % source_path)
      return None
    
    # Create isolated filename: macro_<4-char-guid>_<original_name>
    original_name = os.path.basename(source_path)
    short_guid = uuid.uuid4().hex[:4]
    isolated_name = 'macro_%s_%s' % (short_guid, original_name)
    
    # Destination path
    macros_dir = self.get_macros_dir()
    real_dest = os.path.join(macros_dir, isolated_name)
    
    try:
      # Copy the file
      shutil.copy2(real_source, real_dest)
      self.log.info('Isolated macro: %s -> macros/%s' % (source_path, isolated_name))
      
      # Return the new relative path
      return 'macros/' + isolated_name
      
    except Exception as e:
      self.log.error('Failed to isolate macro: %s' % str(e))
      return None


  def cleanup_orphaned_macros(self, current_paths):
    """
    Remove macro files that are no longer referenced.
    
    Args:
      current_paths: Set of paths currently in use (relative paths like 'macros/...')
    """
    macros_dir = self.get_macros_dir()
    if not os.path.exists(macros_dir):
      return
    
    # Normalize current paths to just filenames
    current_files = set()
    for path in current_paths:
      if path and (path.startswith('macros/') or path.startswith('Home/macros/')):
        current_files.add(os.path.basename(path))
    
    # Check each file in macros directory
    try:
      for filename in os.listdir(macros_dir):
        filepath = os.path.join(macros_dir, filename)
        
        # Skip directories
        if not os.path.isfile(filepath):
          continue
        
        # Only clean up files that look like isolated macros
        if not filename.startswith('macro_'):
          continue
        
        # Remove if not in current paths
        if filename not in current_files:
          try:
            os.remove(filepath)
            self.log.info('Cleaned up orphaned macro file: %s' % filename)
          except Exception as e:
            self.log.error('Failed to remove orphaned macro: %s' % str(e))
            
    except Exception as e:
      self.log.error('Error cleaning orphaned macros: %s' % str(e))


  def _invalidate(self, path):
    if path == self.ctrl.state.get('first_file', ''):
      self._update_first_file()


  def _update_first_file(self):
    # Get GCode files from root upload directory
    upload = self.ctrl.root + '/upload'

    files = []
    for path in os.listdir(upload):
      # Skip protected directories
      if path in self.PROTECTED_DIRS:
        continue
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
    # Check for protected paths
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
    # Check for protected paths
    if self.is_protected_path(path):
      raise HTTPError(403, 'Cannot create directories in protected area')
    
    realpath = self.realpath(path)

    if not os.path.exists(realpath):
      os.makedirs(realpath)
      os.sync()


  def write(self, path, data):
    # Check for protected paths
    if self.is_protected_path(path):
      raise HTTPError(403, 'Cannot write to protected macro files')
    
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
