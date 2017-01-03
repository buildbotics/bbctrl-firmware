import json
import logging
import pkg_resources

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
    "spindle": {},
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
            return self.load_path('config.json')

        except Exception as e:
            log.warning('%s', e)
            return default_config


    def save(self, config):
        self.update(config)

        config['version'] = self.version

        with open('config.json', 'w') as f:
            json.dump(config, f)

        log.info('Saved')


    def encode_cmd(self, index, value, spec):
        if spec['type'] == 'enum': value = spec['values'].index(value)
        elif spec['type'] == 'bool': value = 1 if value else 0
        elif spec['type'] == 'percent': value /= 100.0

        self.ctrl.avr.set(index, spec['code'], value)


    def encode_category(self, index, config, category):
        for key, spec in category.items():
            if key in config:
                self.encode_cmd(index, config[key], spec)


    def encode(self, index, config, tmpl):
        for category in tmpl.values():
            self.encode_category(index, config, category)


    def update(self, config):
        # Motors
        tmpl = self.template['motors']
        for index in range(len(config['motors'])):
            self.encode(index, config['motors'][index], tmpl)

        # Switches
        tmpl = self.template['switches']
        for index in range(len(config['switches'])):
            self.encode_category(index, config['switches'][index], tmpl)

        # Spindle
        tmpl = self.template['spindle']
        self.encode_category('', config['spindle'], tmpl)
