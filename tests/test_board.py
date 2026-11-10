'''Tests for bbctrl.Board: which main board revision the controller acts on,
and whether the host asserts the AVR buffer enable output, which on Rev 15
and earlier is the motor driver enable line.

The I2C bus is faked at its seam: read_word() either answers, raises the
IOError a missing device produces, or returns None as a disabled bus does.
'''

import json
import os

import pytest

from bbctrl import Board

TEMPLATE_PATH = os.path.join(os.path.dirname(__file__), '..', 'src',
                             'resources', 'config-template.json')


class FakeI2C:
    def __init__(self, answers):
        self.answers = list(answers) # One entry per read: value or exception
        self.reads = []

    def read_word(self, addr, reg):
        self.reads.append((addr, reg))
        answer = self.answers.pop(0)
        if isinstance(answer, Exception): raise answer
        return answer


def no_device(): return IOError('I2C read word failed: [Errno 121]')


@pytest.mark.parametrize('detected', [True, False])
@pytest.mark.parametrize('setting', [Board.REV_13, Board.REV_15, Board.REV_16])
def test_explicit_setting_overrides_detection(setting, detected):
    assert Board.resolve(setting, detected) == setting


def test_auto_with_old_power_chip_is_rev_13():
    assert Board.resolve(Board.AUTO, True) == Board.REV_13


def test_auto_without_old_power_chip_assumes_rev_16():
    # Matches upstream behavior; Rev 14/15 owners must select their revision
    assert Board.resolve(Board.AUTO, False) == Board.REV_16


def test_unknown_setting_is_treated_as_auto():
    assert Board.resolve('bogus', True) == Board.REV_13
    assert Board.resolve('bogus', False) == Board.REV_16


@pytest.mark.parametrize('revision, enable', [
    (Board.REV_13, False),
    (Board.REV_15, False),
    (Board.REV_16, True),
])
def test_buffers_enabled_only_where_power_chip_drives_motor_enable(
        revision, enable):
    assert Board.host_enables_buffers(revision) is enable


def test_old_power_chip_detected_when_it_answers_at_0x60():
    i2c = FakeI2C([0x0102])
    assert Board.has_legacy_pwr_chip(i2c)
    assert i2c.reads == [(0x60, 0)]


def test_no_old_power_chip_when_0x60_never_answers():
    i2c = FakeI2C([no_device()] * Board.LEGACY_PWR_PROBES)
    assert not Board.has_legacy_pwr_chip(i2c)
    assert len(i2c.reads) == Board.LEGACY_PWR_PROBES


def test_one_failed_read_does_not_hide_old_power_chip():
    i2c = FakeI2C([no_device(), 0x0102])
    assert Board.has_legacy_pwr_chip(i2c)


def test_disabled_i2c_bus_means_no_old_power_chip():
    # Demo mode, or no I2C port: read_word() returns None without raising
    assert not Board.has_legacy_pwr_chip(FakeI2C([None]))


def test_config_template_offers_exactly_the_board_revisions():
    with open(TEMPLATE_PATH) as f: template = json.load(f)
    entry = template['admin']['board-revision']

    assert entry['values'] == \
        [Board.AUTO, Board.REV_13, Board.REV_15, Board.REV_16]
    assert entry['default'] == Board.AUTO
