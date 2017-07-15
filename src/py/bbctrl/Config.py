import os
import json
import logging
import pkg_resources
import subprocess

import bbctrl

log = logging.getLogger('Config')

default_config = {
    "motors": [
        {"axis": "X"},
        {"axis": "Y"},
        {"axis": "Z"},
        {"axis": "A"},
        ],
    "switches": {},
    "outputs": {},
    "spindle": {},
    "gcode": {},
    }


class Config(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.version = pkg_resources.require('bbctrl')[0].version
        default_config['version'] = self.version

        # Load config template
        with open(bbctrl.get_resource('http/config-template.json'), 'r',
                  encoding = 'utf-8') as f:
            self.template = json.load(f)


    def load_path(self, path):
        with open(path, 'r') as f:
            return json.load(f)


    def load(self):
        try:
            config = self.load_path('config.json')
            config['version'] = self.version

            # Add missing sections
            for key, value in default_config.items():
                if not key in config: config[key] = value

            return config

        except Exception as e:
            log.warning('%s', e)
            return default_config


    def save(self, config):
        self.update(config)

        config['version'] = self.version

        with open('config.json', 'w') as f:
            json.dump(config, f)

        subprocess.check_call(['sync'])

        log.info('Saved')


    def reset(self): os.unlink('config.json')


    def encode_cmd(self, index, value, spec):
        if not 'code' in spec: return

        if spec['type'] == 'enum':
            if value in spec['values']:
                value = spec['values'].index(value)
            else: value = spec['default']

        elif spec['type'] == 'bool': value = 1 if value else 0
        elif spec['type'] == 'percent': value /= 100.0

        self.ctrl.avr.set(index, spec['code'], value)


    def encode_category(self, index, config, category, with_defaults):
        for key, spec in category.items():
            if key in config: value = config[key]
            elif with_defaults: value = spec['default']
            else: continue

            self.encode_cmd(index, value, spec)


    def encode(self, index, config, tmpl, with_defaults):
        for category in tmpl.values():
            self.encode_category(index, config, category, with_defaults)


    def update(self, config, with_defaults = False):
        for name, tmpl in self.template.items():
            if name == 'motors':
                for index in range(len(config['motors'])):
                    self.encode(index, config['motors'][index], tmpl,
                                with_defaults)

            else: self.encode_category('', config.get(name, {}), tmpl,
                                       with_defaults)


    def config_avr(self): self.update(self.load(), True)
