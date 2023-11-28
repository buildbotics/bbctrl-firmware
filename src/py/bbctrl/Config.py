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
import json
import subprocess
import copy
import glob
import re
import socket
import datetime

from . import util

__all__ = ['Config']


class Config(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Config')

        self.values = {}

        try:
            self.version = util.get_version()

            # Load config template
            with open(util.get_resource('http/config-template.json'), 'r',
                      encoding = 'utf-8') as f:
                self.template = json.load(f)

        except Exception: self.log.exception()


    def get(self, name, default = None):
        return self.values.get(name, default)


    def get_index(self, name, index, default = None):
        return self.values.get(name, {}).get(str(index), None)


    def get_path(self):
        root = self.ctrl.get_path()

        try:
            path = root + '/config-v%s.json' % self.version

            # Look for current config version
            if os.path.exists(path): return path

            # Look for most recent versioned config
            versions = []
            pat = re.compile(r'^.*/config-v(\d+\.\d+\.\d+)\.json$')
            for name in glob.glob(root + '/config-v*.json'):
                m = pat.match(name)
                if m: versions.append(util.parse_version(m.groups()[0]))

            current = util.parse_version(self.version)
            versions = list(filter(lambda v: v < current, versions))
            if len(versions):
                return root + '/config-v%d.%d.%d.json' % max(versions)

        except: self.log.exception()

        # Return default config
        return root + '/config.json'


    def load(self):
        path = self.get_path()

        try:
            if os.path.exists(path):
                with open(path, 'r') as f: config = json.load(f)
            else: config = {'version': self.version}

            try:
                self.upgrade(config)
            except Exception: self.log.exception()

        except Exception as e:
            self.log.warning('%s', e)
            config = {'version': self.version}

        self._defaults(config)
        return config


    def _valid_value(self, template, value):
        type = template['type']

        try:
            if type == 'int':   value = int(value)
            if type == 'float': value = float(value)
            if type == 'text':  value = str(value)
            if type == 'bool':  value = bool(value)
        except:
            return False

        if 'values' in template: return value in template['values']

        return True


    def __defaults(self, config, name, template):
        if 'type' in template:
            if (not name in config or
                not self._valid_value(template, config[name])):
                config[name] = template['default']

            elif 'max' in template and template['max'] < config[name]:
                config[name] = template['max']

            elif 'min' in template and config[name] < template['min']:
                config[name] = template['min']

            if template['type'] == 'list':
                if 'index' in template:
                    config = config[name]
                    for i in range(len(template['index'])):
                        if len(config) <= i: config.append({})

                        for name, tmpl in template['template'].items():
                            self.__defaults(config[i], name, tmpl)

        else:
            for name, tmpl in template.items():
                self.__defaults(config, name, tmpl)


    def _defaults(self, config):
        for name, tmpl in self.template.items():
            if not 'type' in tmpl:
                if not name in config: config[name] = {}
                conf = config[name]
            else: conf = config

            self.__defaults(conf, name, tmpl)


    def upgrade(self, config):
        version = util.parse_version(config['version'])

        if version < util.parse_version(self.version):
            self.log.info('Upgrading config from %s to %s' %
                          (version, self.version))

        if version < (0, 2, 4):
            for motor in config['motors']:
                for key in 'max-jerk max-velocity'.split():
                    if key in motor: motor[key] /= 1000

        if version < (0, 3, 4):
            for motor in config['motors']:
                for key in 'max-accel latch-velocity search-velocity'.split():
                    if key in motor: motor[key] /= 1000

        if version < (0, 3, 23):
            if 'tool' in config:
                if 'spindle-type' in config['tool']:
                    type = config['tool']['spindle-type']
                    if type == 'PWM': type = 'PWM Spindle'
                    if type == 'Huanyang': type = 'Huanyang VFD'
                    config['tool']['tool-type'] = type
                    del config['tool']['spindle-type']

                if 'spin-reversed' in config['tool']:
                    reversed = config['tool']['spin-reversed']
                    config['tool']['tool-reversed'] = reversed
                    del config['tool']['spin-reversed']

        if version < (0, 4, 7):
            for motor in config['motors']:
                if 2 < motor.get('idle-current', 0): motor['idle-current'] = 2
                if 'enabled' not in motor:
                    motor['enabled'] = motor.get('power-mode', '') != 'disabled'

        if version < (1, 0, 2):
            io_map           = self.template['io-map']
            io_defaults      = io_map['default']
            config['io-map'] = io = copy.deepcopy(io_defaults)

            def find_io_index(function):
                for i in range(len(io_defaults)):
                    if io_defaults[i]['function'] == function:
                        return i

                raise Exception('IO default "%s" not found' % function)

            def upgrade_io(config, old, new):
                if not old in config: return
                mode  = config.get(old)
                index = find_io_index(new)

                if mode == 'disabled':
                    io[index]['function'] = 'disabled'
                    io[index]['mode']     = io_defaults[index]['mode']
                else:
                    io[index]['function'] = new
                    io[index]['mode']     = mode

                del config[old]

            # Motor switches
            for i in range(len(config['motors'])):
                motor = config['motors'][i]

                for side in ('min', 'max'):
                    name = 'input-motor-%d-%s' % (i, side)
                    upgrade_io(motor, side + '-switch', name)

            # Inputs
            for name in ('estop', 'probe'):
                upgrade_io(config['switches'], name, 'input-' + name)

            # Outputs
            for old, new in (('fault', 'fault'), ('load-1', 'mist'),
                             ('load-2', 'flood')):
                upgrade_io(config['outputs'], old, 'output-' + new)

            # Tool outputs
            for name in ('tool-direction', 'tool-enable'):
                upgrade_io(config['tool'], name + '-mode', 'output-' + name)

        config['version'] = self.version


    def save(self, config):
        self.upgrade(config)
        self._update(config, False)

        path = self.ctrl.get_path('config-v%s.json' % self.version)
        with open(path, 'w') as f:
            json.dump(config, f)

        os.sync()

        self.ctrl.events.emit('invalidate-all')
        self.log.info('Saved')


    def reset(self):
        config = {'version': self.version}
        self.save(config)
        self._defaults(config)
        self._update(config, True)
        self.ctrl.events.emit('invalidate-all')


    def get_filename(self):
        fmt = socket.gethostname() + '-%Y%m%d-%H%M%S.json'
        return datetime.datetime.now().strftime(fmt)


    def backup(self):
        path = 'upload/configs'
        if not os.path.exists(path): os.makedirs(path)
        path += '/' + self.get_filename()

        with open(path, 'w') as f:
            json.dump(self.load(), f, indent = 2, separators = (',', ': '))


    def _encode(self, name, index, config, tmpl, with_defaults):
        # Handle category
        if not 'type' in tmpl:
            for name, entry in tmpl.items():
                if 'type' in entry and config is not None:
                    conf = config.get(name, None)
                else: conf = config
                self._encode(name, index, conf, entry, with_defaults)
            return

        # Handle defaults
        if config is not None:
            if self._valid_value(tmpl, config): value = config
            else: value = tmpl['default']

        elif with_defaults: value = tmpl['default']
        else: return

        # Handle list
        if tmpl['type'] == 'list':
            if 'index' in tmpl:
                for i in range(len(tmpl['index'])):
                    if config is not None and i < len(config): conf = config[i]
                    else: conf = None
                    self._encode(name, index + str(tmpl['index'][i]), conf,
                                 tmpl['template'], with_defaults)
            else: self.values[name] = value
            return

        # Update config values
        if index:
            if not name in self.values: self.values[name] = {}
            self.values[name][index] = value

        else: self.values[name] = value

        # Update state variable
        if not 'code' in tmpl: return

        if tmpl['type'] == 'enum':
            if value in tmpl['values']: value = tmpl['values'].index(value)
            else: value = tmpl['default']

        elif tmpl['type'] == 'bool': value = 1 if value else 0
        elif tmpl['type'] == 'percent': value /= 100.0

        self.ctrl.state.config(index + tmpl['code'], value)


    def _update(self, config, with_defaults):
        for name, tmpl in self.template.items():
            conf = config.get(name, None)
            self._encode(name, '', conf, tmpl, with_defaults)


    def reload(self): self._update(self.load(), True)
