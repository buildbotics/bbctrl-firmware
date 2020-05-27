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
import json
import pkg_resources
import subprocess
import copy
from pkg_resources import Requirement, resource_filename


def get_resource(path):
    return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


class Config(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Config')

        self.values = {}

        try:
            self.version = pkg_resources.require('bbctrl')[0].version

            # Load config template
            with open(get_resource('http/config-template.json'), 'r',
                      encoding = 'utf-8') as f:
                self.template = json.load(f)

        except Exception: self.log.exception()


    def get(self, name, default = None):
        return self.values.get(name, default)


    def get_index(self, name, index, default = None):
        return self.values.get(name, {}).get(str(index), None)


    def load(self):
        path = self.ctrl.get_path('config.json')

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

        if 'values' in template and value not in template['values']:
            return False

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
        version = tuple(map(int, config['version'].split('.')))

        if version < (0, 2, 4):
            for motor in config['motors']:
                for key in 'max-jerk max-velocity'.split():
                    if key in motor: motor[key] /= 1000

        if version < (0, 3, 4):
            for motor in config['motors']:
                for key in 'max-accel latch-velocity search-velocity'.split():
                    if key in motor: motor[key] /= 1000

        if version <= (0, 3, 22):
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

        if version <= (0, 4, 6):
            for motor in config['motors']:
                if 2 < motor.get('idle-current', 0): motor['idle-current'] = 2
                if 'enabled' not in motor:
                    motor['enabled'] = motor.get('power-mode', '') != 'disabled'

        config['version'] = self.version


    def save(self, config):
        self.upgrade(config)
        self._update(config, False)

        with open(self.ctrl.get_path('config.json'), 'w') as f:
            json.dump(config, f)

        os.sync()

        self.ctrl.preplanner.invalidate_all()
        self.log.info('Saved')


    def reset(self):
        if os.path.exists('config.json'): os.unlink('config.json')
        self.reload()
        self.ctrl.preplanner.invalidate_all()


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
        if config is not None: value = config
        elif with_defaults: value = tmpl['default']
        else: return

        # Handle list
        if tmpl['type'] == 'list':
            for i in range(len(tmpl['index'])):
                if config is not None and i < len(config): conf = config[i]
                else: conf = None
                self._encode(name, index + tmpl['index'][i], conf,
                             tmpl['template'], with_defaults)
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
