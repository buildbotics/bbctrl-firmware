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
import json
import logging
import pkg_resources
import subprocess
import copy
from pkg_resources import Requirement, resource_filename

log = logging.getLogger('Config')


def get_resource(path):
    return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


class Config(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.values = {}

        try:
            self.version = pkg_resources.require('bbctrl')[0].version

            # Load config template
            with open(get_resource('http/config-template.json'), 'r',
                      encoding = 'utf-8') as f:
                self.template = json.load(f)

        except Exception as e: log.exception(e)


    def get(self, name, default = None):
        return self.values.get(name, default)


    def get_index(self, name, index, default = None):
        return self.values.get(name, {}).get(str(index), None)


    def load_path(self, path):
        with open(path, 'r') as f:
            return json.load(f)


    def load(self, path = 'config.json'):
        try:
            if os.path.exists(path): config = self.load_path(path)
            else: config = {'version': self.version}

            try:
                self.upgrade(config)
            except Exception as e: log.exception(e)

        except Exception as e:
            log.warning('%s', e)
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

        if (('min' in template and value < template['min']) or
            ('max' in template and template['max'] < value) or
            ('values' in template and value not in template['values'])):
            return False

        return True


    def __defaults(self, config, name, template):
        if 'type' in template:
            if (not name in config or
                not self._valid_value(template, config[name])):
                config[name] = template['default']

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

        config['version'] = self.version


    def save(self, config):
        self.upgrade(config)
        self._update(config, False)

        with open('config.json', 'w') as f:
            json.dump(config, f)

        subprocess.check_call(['sync'])

        self.ctrl.preplanner.invalidate_all()
        log.info('Saved')


    def reset(self):
        os.unlink('config.json')
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



if __name__ == "__main__":
    import sys
    import argparse

    class State(object):
        def config(self, name, value): print('config(%s, %s)' % (name, value))


    class Ctrl(object):
        def __init__(self):
            self.state = State()

    parser = argparse.ArgumentParser(description = 'Buildbotics Config Test')

    parser.add_argument('configs', metavar = 'CONFIG', nargs = '*',
                        help = 'Configuration file')
    parser.add_argument('-u', '--update', action = 'store_true',
                        help = 'Update config')
    args = parser.parse_args()

    config = Config(Ctrl())

    def do_cfg(path):
        cfg = config.load(path)
        if args.update: config._update(cfg, True)
        else: print(json.dumps(cfg, sort_keys = True, indent = 2))

    if len(args.configs):
        for path in args.configs: do_cfg(path)

    else: do_cfg('')
