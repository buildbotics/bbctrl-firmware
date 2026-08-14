'''Tests for dual-motor (gantry squaring) homing.

The controller can drive two motors from one axis.  When both carry their own
limit switch the axis homes each motor to its own switch, squaring the gantry.
These tests cover the Python half of that: which motors an axis owns, whether
it should square, which switch IDs go to the AVR, and how the seek command is
encoded.
'''

import os
import re

import pytest

from bbctrl.State import State
from bbctrl import Cmd


# Homing mode codes, per State.motor_homing_mode()
MANUAL, SWITCH_MIN, SWITCH_MAX, STALL_MIN, STALL_MAX = range(5)

AVR_SRC = os.path.join(os.path.dirname(__file__), '..', 'src', 'avr', 'src')


def motor_vars(index, axis, enabled = 1, homing = SWITCH_MIN, square = 0,
               soft_min = 0, soft_max = 100):
    '''Config variables for a single motor, as Config.py would set them.'''
    return {
        '%dan' % index: 'xyzabc'.index(axis),  # axis mapping
        '%dme' % index: enabled,               # motor enabled
        '%dho' % index: homing,                # homing mode
        '%dsq' % index: square,                # square-axis
        '%dtn' % index: soft_min,              # min soft limit
        '%dtm' % index: soft_max,              # max soft limit
    }


def make_state(*motors):
    '''A State carrying only config vars, with no controller behind it.'''
    state = State.__new__(State)
    state.vars = {}
    state.callbacks = {}
    for m in motors: state.vars.update(m)
    return state


@pytest.fixture
def squared_y():
    '''Y driven by motors 1 and 3, squaring enabled, homing to min.'''
    return make_state(motor_vars(0, 'x'),
                      motor_vars(1, 'y', square = 1),
                      motor_vars(3, 'y'))


# --- which motors an axis owns ---------------------------------------------

def test_find_motors_returns_nothing_for_unmapped_axis(squared_y):
    assert squared_y.find_motors('z') == []
    assert squared_y.find_motor('z') is None


def test_find_motors_returns_single_motor_axis(squared_y):
    assert squared_y.find_motors('x') == [0]
    assert squared_y.find_motor('x') == 0


def test_find_motors_returns_both_motors_in_motor_order(squared_y):
    assert squared_y.find_motors('y') == [1, 3]
    assert squared_y.find_motor('y') == 1


def test_find_motors_skips_disabled_motors():
    state = make_state(motor_vars(1, 'y'), motor_vars(3, 'y', enabled = 0))
    assert state.find_motors('y') == [1]


def test_find_motors_handles_three_motors_on_one_axis():
    state = make_state(motor_vars(1, 'y'), motor_vars(2, 'y'),
                       motor_vars(3, 'y'))
    assert state.find_motors('y') == [1, 2, 3]


# --- when an axis should square --------------------------------------------

def test_axis_is_squared_when_configured_with_two_motors(squared_y):
    assert squared_y.axis_is_squared('y') is True


def test_axis_is_not_squared_when_flag_is_off():
    state = make_state(motor_vars(1, 'y', square = 0), motor_vars(3, 'y'))
    assert state.axis_is_squared('y') is False


def test_axis_is_not_squared_with_only_one_motor():
    state = make_state(motor_vars(1, 'y', square = 1))
    assert state.axis_is_squared('y') is False


def test_axis_is_not_squared_when_second_motor_is_disabled():
    state = make_state(motor_vars(1, 'y', square = 1),
                       motor_vars(3, 'y', enabled = 0))
    assert state.axis_is_squared('y') is False


def test_axis_is_not_squared_in_stall_homing_mode():
    '''Stall homing has no switch to hold each motor against.'''
    state = make_state(motor_vars(1, 'y', square = 1, homing = STALL_MIN),
                       motor_vars(3, 'y', homing = STALL_MIN))
    assert state.axis_is_squared('y') is False


def test_unsquared_axis_is_not_squared(squared_y):
    assert squared_y.axis_is_squared('x') is False


# --- switch IDs sent to the AVR --------------------------------------------

def test_squared_axis_sends_both_motor_switches(squared_y):
    # Motor 1 min -> INPUT_MOTOR_1_MIN (4), motor 3 min -> INPUT_MOTOR_3_MIN (8)
    assert squared_y.get_switch_id('y-min') == 4
    assert squared_y.get_square_switch_id('y-min') == 8


def test_squared_axis_uses_max_switches_when_homing_to_max():
    state = make_state(motor_vars(1, 'y', square = 1, homing = SWITCH_MAX),
                       motor_vars(3, 'y', homing = SWITCH_MAX))
    assert state.get_switch_id('y-max') == 5   # INPUT_MOTOR_1_MAX
    assert state.get_square_switch_id('y-max') == 9   # INPUT_MOTOR_3_MAX


def test_second_motor_follows_the_first_motors_homing_mode():
    '''Squaring homes both motors to the same side, set on the first motor.

    The second motor's own homing-mode is deliberately different here and must
    be ignored, or the two motors would seek opposite ends of the axis.
    '''
    state = make_state(motor_vars(1, 'y', square = 1, homing = SWITCH_MAX),
                       motor_vars(3, 'y', homing = SWITCH_MIN))
    assert state.get_square_switch_id('y-max') == 9  # max, not min (8)


def test_single_motor_axis_sends_no_second_switch(squared_y):
    assert squared_y.get_switch_id('x-min') == 2     # INPUT_MOTOR_0_MIN
    assert squared_y.get_square_switch_id('x-min') is None


def test_probe_has_no_second_switch(squared_y):
    assert squared_y.get_switch_id('probe') == 1
    assert squared_y.get_square_switch_id('probe') is None


def test_unsupported_switch_name_is_rejected(squared_y):
    with pytest.raises(Exception):
        squared_y.get_switch_id('y-middle')


def test_stall_homing_uses_stall_input_ids():
    state = make_state(motor_vars(2, 'z', homing = STALL_MIN))
    assert state.get_switch_id('z-min') == 12        # INPUT_STALL_2


def test_switch_ids_match_the_avr_decode_table():
    '''State.motor_switch_id() must agree with _decode_id() in avr/src/seek.c.

    These are two hand-maintained halves of one wire protocol.  If the AVR
    table is renumbered without updating Python, homing would seek the wrong
    motor's switch, so pin the mapping to the firmware source itself.
    '''
    with open(os.path.join(AVR_SRC, 'seek.c')) as f: source = f.read()

    decode = dict((int(num), name) for num, name in
                  re.findall(r'case\s+(\d+):\s*return\s+(\w+);', source))

    state = make_state(*[motor_vars(m, 'xyzabc'[m]) for m in range(4)])

    for m in range(4):
        axis = 'xyzabc'[m]
        for side in ('min', 'max'):
            got = state.motor_switch_id(m, side, 'switch-' + side)
            assert decode[got] == 'INPUT_MOTOR_%d_%s' % (m, side.upper())

        stalled = state.motor_switch_id(m, 'min', 'stall-min')
        assert decode[stalled] == 'INPUT_STALL_%d' % m


# --- configuration errors surfaced before the machine moves ----------------

def test_squaring_without_a_second_motor_is_reported():
    state = make_state(motor_vars(1, 'y', square = 1))
    reason = state.axis_home_fail_reason('y')
    assert reason is not None and 'second' in reason


def test_squaring_with_stall_homing_is_reported():
    state = make_state(motor_vars(1, 'y', square = 1, homing = STALL_MIN),
                       motor_vars(3, 'y', homing = STALL_MIN))
    reason = state.axis_home_fail_reason('y')
    assert reason is not None and 'switch-min' in reason


def test_correctly_squared_axis_reports_no_failure(squared_y):
    assert squared_y.axis_home_fail_reason('y') is None


def test_bad_soft_limits_still_reported_on_a_squared_axis():
    state = make_state(motor_vars(1, 'y', square = 1, soft_min = 0,
                                  soft_max = 0),
                       motor_vars(3, 'y'))
    assert 'soft-limit' in state.axis_home_fail_reason('y')


# --- seek command encoding -------------------------------------------------

def test_seek_encodes_a_single_switch():
    assert Cmd.seek(4, True, True) == 's43'


def test_seek_appends_the_second_switch_when_squaring():
    assert Cmd.seek(4, True, True, 8) == 's438'


def test_seek_omits_the_second_switch_when_not_squaring():
    '''None must not encode; old firmware reads a 4th character as a switch.'''
    assert Cmd.seek(4, True, True, None) == 's43'


def test_seek_flag_bits():
    assert Cmd.seek(2, False, False) == 's20'
    assert Cmd.seek(2, True, False) == 's21'
    assert Cmd.seek(2, False, True) == 's22'


def test_seek_round_trips_through_decode():
    decoded = Cmd.decode_command(Cmd.seek(4, True, False, 8))
    assert decoded['type'] == 'seek'
    assert decoded['port'] == 4
    assert decoded['port2'] == 8
    assert decoded['active'] is True
    assert decoded['error'] is False


def test_single_switch_seek_decodes_without_a_second_port():
    decoded = Cmd.decode_command(Cmd.seek(9, False, True))
    assert decoded['port'] == 9
    assert 'port2' not in decoded
    assert decoded['active'] is False
    assert decoded['error'] is True
