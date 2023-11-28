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
import re
import subprocess
import copy
import json
import configparser
from tornado import gen, process, iostream

__all__ = ['Network']


def escaped_split(s, delims = ' \t\n\r', escapes = '\\'):
  part = []
  esc  = None

  for c in s:
    if esc is not None:
      esc = None
      part.append(c)

    elif c in escapes: esc = c
    elif c in delims:
      if len(part):
        yield ''.join(part)
        part = []

    else: part.append(c)

  if esc is not None: part.append(esc)
  if len(part): yield ''.join(part)


def read_file(path):
  try:
    with open(path, 'r') as f:
      return f.read().strip()

  except Exception as e: pass


def parse_rows(s, fields, types):
  if isinstance(fields, str): fields = fields.split(',')

  for line in s.split('\n'):
    if line:
      parts = list(escaped_split(line, ':'))

      if len(parts) == len(fields):
        yield {fields[i]: types[i](parts[i]) for i in range(len(fields))}


def parse_fields(s):
  d = dict()

  for line in s.split('\n'):
    if line:
      key, value = line.split(':', maxsplit = 1)
      key = key.lower()

      if '[' in key:
        key = re.sub(r'\[\d+\]', '', key)
        if not key in d: d[key] = []
        d[key].append(value)

      else: d[key] = value

  return d


class Network:
  def __init__(self, ctrl):
    self.ctrl     = ctrl
    self.log      = ctrl.log.get('Net')
    self.channels = {}

    if not ctrl.args.demo:
      ctrl.ioloop.add_callback(self._monitor)


  @gen.coroutine
  def _monitor(self):
    self.ctrl.ioloop.add_callback(self._load_devices)

    cmd = ['nmcli', '-t', 'monitor']

    while True:
      proc = process.Subprocess(cmd, stdout = process.Subprocess.STREAM)

      try:
        while True:
          line = yield proc.stdout.read_until(b'\n')
          line = line.decode('utf-8').strip()

          if ':' in line:
            name, value = line.split(':', maxsplit = 1)
            value = value.strip()

            if value == 'device created':
              self._add_dev(name)
              self._set_devs()

            if name in self.devs:
              self.log.info('Mon: ' + line)

              if value == 'device removed':
                del self.devs[name]
                self._set_devs()
                continue

              dev   = self.devs[name]
              state = value.split()[0]

              if state in ('disconnected', 'connected', 'connecting'):
                if state != dev['state']:
                  self._add_dev(name)
                  self.devs[name]['state'] = state
                  self._set_devs()

      except iostream.StreamClosedError: pass
      except: self.log.exception('nmcli monitor')

      finally:
        proc.stdout.close()
        yield proc.wait_for_exit(False)


  def _exec(self, cmd, **kwargs):
    self.log.info(' '.join(cmd))

    result = subprocess.run(cmd, **kwargs)

    if result.returncode and 'capture_output' in kwargs:
      self.log.error(result.stderr)

    return result


  def _nmcli(self, cmd, **kwargs):
    return self._exec(['nmcli', '-t'] + cmd, **kwargs)


  def scan(self, name = '*'):
    changed = False

    for dev in self.devs.values():
      if name == '*' or dev['name'] == name and dev['type'] == 'wifi':
        scan = self._dev_scan(dev['name'])

        if not 'scan' in dev or json.dumps(scan) != json.dumps(dev['scan']):
          dev['scan'] = scan
          changed = True

    if changed: self._set_devs()


  def connect(self, device, uuid = None, ssid = None, password = None):
    if uuid: cmd = ['connection', 'up', 'uuid', uuid]
    else:
      cmd = ['device', 'wifi', 'connect', ssid]
      if password is not None: cmd += ['password', password]

    self._nmcli(['-w', '0'] + cmd + ['ifname', device])


  def forget(self, device, uuid):
    self._nmcli(['connection', 'delete', 'uuid', uuid])


  def disconnect(self, device):
    self._nmcli(['device', 'disconnect', device])


  def _set_devs(self):
    self.ctrl.state.set('network', copy.deepcopy(list(self.devs.values())))


  def _load_scan(self, name):
    types  = [str, int, str, int, str]
    fields = 'ssid,chan,rate,signal,security'
    cmd    = ['-f', fields, 'device', 'wifi', 'list', 'ifname', name]
    result = self._nmcli(cmd, capture_output = True, text = True)

    yield from parse_rows(result.stdout, fields, types)


  def _dev_scan(self, name): return list(self._load_scan(name))


  def _get_phy(self, name):
    return read_file('/sys/class/net/' + name + '/phy80211/name')


  def _load_channels(self, phy):
    cmd    = ['iw', 'phy', phy, 'channels']
    result = self._exec(cmd, capture_output = True, text = True)
    chRE   = re.compile(r'^.*?\[(\d+)\].*$')

    for line in result.stdout.split('\n'):
      m = chRE.match(line)
      if m and not 'disabled' in line:
        yield int(m.group(1))


  def _dev_channels(self, name):
    phy = self._get_phy(name)
    if not phy: return

    if not phy in self.channels:
      self.channels[phy] = list(self._load_channels(phy))

    return self.channels[phy]


  def _load_connection(self, uuid):
    cmd    = ['connection', 'show', 'uuid', uuid]
    result = self._nmcli(cmd, capture_output = True, text = True)
    conf   = parse_fields(result.stdout)

    return dict(
      ssid    = conf.get('802-11-wireless.ssid'),
      uuid    = conf.get('connection.uuid'),
      active  = conf.get('general.state') == 'activated',
      hotspot = conf.get('802-11-wireless.mode') == 'ap',
    )


  def _load_dev(self, name):
    dev    = dict(name = name, connections = {})
    cmd    = ['-f', 'general,ip4,connections', 'device', 'show', name]
    result = self._nmcli(cmd, capture_output = True, text = True)
    conf   = parse_fields(result.stdout)

    dev['type'] = conf['general.type']
    if not dev['type'] in ['wifi', 'ethernet']: return

    dev['state'] = conf['general.state'].split('(')[1].split(')')[0]
    dev['ipv4']  = [
      re.sub(r'/\d+', '', addr) for addr in conf.get('ip4.address', [])]

    if dev['type'] == 'wifi':
      dev['scan']     = self._dev_scan(name)
      dev['channels'] = self._dev_channels(name)

    for value in conf.get('connections.available-connections', []):
      uuid = value.split()[0]
      con  = self._load_connection(uuid)
      dev['connections'][uuid] = con

      if con['active']: dev['ssid'] = con['ssid']

      if not con['hotspot']:
        for e in dev.get('scan', []):
          if e['ssid'] == con['ssid']:
            e['connection'] = con['uuid']

    # Is USB device?
    dev['usb'] = 'usb' in conf.get('general.udi')

    return dev


  def _add_dev(self, name):
    dev = self._load_dev(name)
    if dev: self.devs[name] = dev


  def _load_devices(self):
    self.devs = {}

    for name in os.listdir('/sys/class/net'):
      self._add_dev(name)

    self._set_devs()
