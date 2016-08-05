import json
import logging

import bbctrl


log = logging.getLogger('Config')


class Config(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl

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
            return self.load_path(
                bbctrl.get_resource('http/default-config.json'))


    def save(self, config):
        with open('config.json', 'w') as f:
            json.dump(config, f)

        self.update(config)

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
            self.encode(index + 1, config['motors'][index], tmpl)

        # Axes
        tmpl = self.template['axes']
        for axis in 'xyzabc':
            if not axis in config['axes']: continue
            self.encode(axis, config['axes'][axis], tmpl)

        # Switches
        tmpl = self.template['switches']
        for index in range(len(config['switches'])):
            self.encode_category(index + 1, config['switches'][index], tmpl)

        # Spindle
        tmpl = self.template['spindle']
        self.encode_category('', config['spindle'], tmpl)
