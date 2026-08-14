'''Round-trip and validation tests for bbctrl.Config.

Config loads config-template.json (the single source of truth for settings,
defaults, and AVR variable codes), fills defaults, validates and clamps
values, upgrades old config versions, and persists config-v<version>.json.

The controller behind Config is faked at its seams: get_path() points into a
pytest tmp_path and state.config() records the AVR variable codes that a real
controller would send to the firmware.
'''

import json
import os

import pytest

from bbctrl.Config import Config
from bbctrl import util

TEMPLATE_PATH = os.path.join(os.path.dirname(__file__), '..', 'src',
                             'resources', 'config-template.json')


class FakeLog:
    def get(self, name): return self
    def info(self, *args, **kwargs): pass
    def warning(self, *args, **kwargs): pass
    def exception(self, *args, **kwargs): pass


class FakeEvents:
    def __init__(self): self.emitted = []
    def emit(self, event, *args): self.emitted.append(event)


class FakeState:
    def __init__(self): self.codes = {}
    def config(self, code, value): self.codes[code] = value


class FakeCtrl:
    def __init__(self, root):
        # Forward slashes: Config.get_path()'s version regex expects them
        self.root   = str(root).replace('\\', '/')
        self.state  = FakeState()
        self.events = FakeEvents()
        self.log    = FakeLog()

    def get_path(self, path = None):
        return self.root + ('/' + path if path else '')


@pytest.fixture
def config(tmp_path, monkeypatch):
    # os.sync() does not exist on all platforms (e.g. Windows dev hosts)
    monkeypatch.setattr(os, 'sync', lambda: None, raising = False)

    ctrl = FakeCtrl(tmp_path)

    c = Config.__new__(Config)
    c.ctrl = ctrl
    c.log = FakeLog()
    c.values = {}
    c.version = util.get_version()
    with open(TEMPLATE_PATH, encoding = 'utf-8') as f:
        c.template = json.load(f)

    return c


# --- defaults ---------------------------------------------------------------

def test_load_without_a_file_returns_full_defaults(config):
    loaded = config.load()

    assert loaded['version'] == config.version
    assert len(loaded['motors']) == 4
    assert loaded['motors'][0]['homing-mode'] == 'manual'
    assert loaded['settings']['units'] == 'METRIC'


def test_defaults_include_squaring_settings(config):
    motor = config.load()['motors'][0]

    assert motor['square-axis'] is False
    assert motor['max-square-error'] == 10


# --- save / load round trip -------------------------------------------------

def test_save_then_load_round_trips_motor_settings(config):
    data = config.load()
    data['motors'][1]['travel-per-rev'] = 10
    data['motors'][1]['axis'] = 'Y'
    data['motors'][3]['axis'] = 'Y'
    data['motors'][1]['square-axis'] = True

    config.save(data)
    loaded = config.load()

    assert loaded['motors'][1]['travel-per-rev'] == 10
    assert loaded['motors'][1]['axis'] == 'Y'
    assert loaded['motors'][3]['axis'] == 'Y'
    assert loaded['motors'][1]['square-axis'] is True
    # Untouched motors keep their defaults
    assert loaded['motors'][0]['square-axis'] is False


def test_save_writes_versioned_file_and_invalidates(config):
    config.save(config.load())

    path = config.ctrl.get_path('config-v%s.json' % config.version)
    assert os.path.exists(path)
    assert 'invalidate-all' in config.ctrl.events.emitted


@pytest.mark.skipif(os.sep != '/', reason = 'Config.get_path() matches '
                    'POSIX separators from glob(); controller is Linux')
def test_load_prefers_most_recent_older_version(config):
    old = {'version': '0.9.9'}
    with open(config.ctrl.get_path('config-v0.9.9.json'), 'w') as f:
        json.dump(old, f)

    assert config.get_path().endswith('config-v0.9.9.json')


def test_corrupt_config_file_falls_back_to_defaults(config):
    with open(config.ctrl.get_path('config.json'), 'w') as f:
        f.write('{not json')

    loaded = config.load()
    assert loaded['motors'][0]['homing-mode'] == 'manual'


# --- validation and clamping ------------------------------------------------

def test_out_of_range_values_are_clamped_on_load(config):
    data = config.load()
    data['input']['input-debounce'] = 999999   # max is 5000
    config.save(data)

    assert config.load()['input']['input-debounce'] == 5000


def test_below_minimum_values_are_clamped_on_load(config):
    data = config.load()
    data['input']['input-debounce'] = 0        # min is 1
    config.save(data)

    assert config.load()['input']['input-debounce'] == 1


def test_invalid_enum_value_falls_back_to_default(config):
    data = config.load()
    data['settings']['units'] = 'FURLONGS'
    config.save(data)

    assert config.load()['settings']['units'] == 'METRIC'


# --- version upgrades -------------------------------------------------------

def test_upgrade_rescales_old_motor_velocities(config):
    # switches/outputs/tool are required by the <1.0.2 io-map migration
    old = {'version': '0.2.0',
           'motors': [{'max-velocity': 5000, 'search-velocity': 500}],
           'switches': {}, 'outputs': {}, 'tool': {}}

    config.upgrade(old)

    # <0.2.4 divides max-velocity by 1000, <0.3.4 divides search-velocity
    assert old['motors'][0]['max-velocity'] == 5
    assert old['motors'][0]['search-velocity'] == 0.5
    assert old['version'] == config.version


def test_upgrade_renames_spindle_type(config):
    old = {'version': '0.3.10', 'motors': [],
           'tool': {'spindle-type': 'Huanyang'},
           'switches': {}, 'outputs': {}}

    config.upgrade(old)

    assert old['tool']['tool-type'] == 'Huanyang VFD'
    assert 'spindle-type' not in old['tool']


def test_current_version_upgrade_is_a_no_op(config):
    data = config.load()
    before = json.dumps(data, sort_keys = True)
    config.upgrade(data)
    assert json.dumps(data, sort_keys = True) == before


# --- AVR variable codes -----------------------------------------------------

def test_save_sends_motor_codes_to_state(config):
    data = config.load()
    data['motors'][2]['travel-per-rev'] = 8
    config.save(data)

    codes = config.ctrl.state.codes
    assert codes['2tr'] == 8


def test_save_encodes_enum_and_bool_codes_as_integers(config):
    data = config.load()
    data['motors'][0]['homing-mode'] = 'switch-min'
    data['motors'][0]['square-axis'] = True
    config.save(data)

    codes = config.ctrl.state.codes
    assert codes['0ho'] == 1   # enum index of switch-min
    assert codes['0sq'] == 1   # bool as int
    assert codes['0qe'] == 10  # max-square-error default
