'''Round-trip tests for the GCode/JSON command protocol in bbctrl.Cmd.

Cmd.py encodes planner output into the single-letter serial commands the AVR
firmware parses (command.def) and decodes them back for debugging.  Floats
cross the wire as base64-packed little-endian float32, so all comparisons here
allow float32 precision.
'''

import math
import struct

import pytest

from bbctrl import Cmd


def f32(x):
    '''x rounded through float32, the wire precision.'''
    return struct.unpack('<f', struct.pack('<f', x))[0]


# --- float encoding ---------------------------------------------------------

@pytest.mark.parametrize('value', [
    0.0, 1.0, -1.0, 0.5, -1.5, 0.001, 123456.789, 1e-6, -3.3e4])
def test_float_round_trips_at_float32_precision(value):
    assert Cmd.decode_float(Cmd.encode_float(value)) == pytest.approx(
        f32(value), rel = 1e-6)


def test_float_encoding_is_six_characters():
    # The AVR parser and decode_command() both assume fixed-width fields
    for value in (0.0, -1.5, 8e37):
        assert len(Cmd.encode_float(value)) == 6


def test_infinite_floats_survive_encoding():
    assert Cmd.decode_float(Cmd.encode_float(math.inf)) == math.inf
    assert Cmd.decode_float(Cmd.encode_float(-math.inf)) == -math.inf


# --- axis vectors -----------------------------------------------------------

def test_encode_axes_empty():
    assert Cmd.encode_axes({}) == ''


def test_encode_axes_single():
    data = Cmd.encode_axes({'x': 1.5})
    assert data[0] == 'x' and len(data) == 7
    assert Cmd.decode_float(data[1:]) == 1.5


def test_encode_axes_emits_fixed_axis_order():
    # Axis order on the wire must be stable regardless of dict order
    data = Cmd.encode_axes({'z': 3.0, 'x': 1.0, 'a': 4.0, 'y': 2.0})
    assert [data[i] for i in range(0, len(data), 7)] == ['x', 'y', 'z', 'a']


def test_encode_axes_accepts_uppercase_names():
    data = Cmd.encode_axes({'X': 2.5})
    assert data[0] == 'x'
    assert Cmd.decode_float(data[1:]) == 2.5


def test_encode_axes_all_six():
    axes = dict(zip('xyzabc', (1.0, 2.0, 3.0, 4.0, 5.0, 6.0)))
    assert len(Cmd.encode_axes(axes)) == 6 * 7


# --- set commands -----------------------------------------------------------

def test_set_int_uses_async_marker():
    assert Cmd.set('0me', 1) == '$0me=1'


def test_set_sync_int_uses_sync_marker():
    assert Cmd.set_sync('id', 42) == '#id=42'


def test_set_float_always_syncs_and_base64_encodes():
    cmd = Cmd.set('vm', 2.5)
    assert cmd.startswith('#vm=:')
    assert Cmd.decode_float(cmd[5:]) == 2.5


def test_set_axis_round_trips():
    cmd = Cmd.set_axis('y', -12.75)
    assert cmd[0] == Cmd.SET_AXIS and cmd[1] == 'y'
    assert Cmd.decode_float(cmd[2:]) == -12.75


# --- line commands ----------------------------------------------------------

def test_line_round_trips_through_decoder():
    target = {'x': 10.0, 'y': -5.5}
    times = [0, 0.5, 0, 1.25, 0, 0, 0.75]

    cmd = Cmd.line(target, 100.0, 5000.0, 200.0, times, [])
    decoded = Cmd.decode_command(cmd)

    assert decoded['type'] == 'line'
    assert decoded['exit-vel'] == pytest.approx(100.0)
    assert decoded['max-accel'] == pytest.approx(5000.0)
    assert decoded['max-jerk'] == pytest.approx(200.0)
    assert decoded['target'] == {
        'x': pytest.approx(10.0), 'y': pytest.approx(-5.5)}

    # Times are sent in minutes (input in ms)
    for i, t in enumerate(times):
        assert decoded['times'][i] == pytest.approx(t / 60000, abs = 1e-9)


def test_line_zero_times_are_omitted_from_the_wire():
    lengths = len(Cmd.line({'x': 1.0}, 0, 0, 0, [0] * 7, []))
    assert lengths == 1 + 3 * 6 + 7  # code + 3 floats + 1 axis


def test_line_appends_sync_speeds_on_separate_lines():
    cmd = Cmd.line({'x': 1.0}, 0, 0, 0, [0] * 7, [(5.0, 12000.0)])
    lines = cmd.split('\n')

    assert len(lines) == 2
    speed = Cmd.decode_command(lines[1])
    assert speed['type'] == 'speed'
    assert speed['offset'] == pytest.approx(5.0)
    assert speed['speed'] == pytest.approx(12000.0)


# --- I/O commands -----------------------------------------------------------

def test_input_command_encodes_type_index_and_mode():
    cmd = Cmd.input('digital-in-2', 'rise', 3.0)
    assert cmd.startswith('Id21')
    assert Cmd.decode_float(cmd[4:]) == 3.0


def test_analog_input_command():
    assert Cmd.input('analog-in-1', 'immediate', 0.0).startswith('Ia10')


def test_unknown_input_port_is_rejected():
    with pytest.raises(Exception, match = 'Unsupported input'):
        Cmd.input('digital-in-9', 'rise', 1.0)


def test_unknown_input_mode_is_rejected():
    with pytest.raises(Exception, match = 'Unsupported input mode'):
        Cmd.input('digital-in-0', 'sideways', 1.0)


@pytest.mark.parametrize('port,id', [
    ('digital-out-0', '0'), ('digital-out-3', '3'),
    ('mist', 'M'), ('flood', 'F')])
def test_output_command_ids(port, id):
    assert Cmd.output(port, True) == '#%soa=1' % id
    assert Cmd.output(port, 0) == '#%soa=0' % id


def test_unknown_output_port_is_rejected():
    with pytest.raises(Exception, match = 'Unsupported output'):
        Cmd.output('digital-out-7', True)


# --- misc commands ----------------------------------------------------------

def test_pause_types():
    assert Cmd.pause('program') == 'P1'
    assert Cmd.pause('optional') == 'P2'
    assert Cmd.pause('pallet-change') == 'P1'

    with pytest.raises(Exception, match = 'Unknown pause type'):
        Cmd.pause('coffee')


def test_modbus_commands():
    assert Cmd.modbus_read(258) == 'm258'
    assert Cmd.modbus_write(7, 1500) == 'M7=1500'


def test_jog_command_encodes_id_and_axes():
    cmd = Cmd.jog(0x1a2b, {'x': 0.5, 'z': -0.25})

    assert cmd.startswith('j1a2b')
    assert cmd[5] == 'x' and Cmd.decode_float(cmd[6:12]) == 0.5
    assert cmd[12] == 'z' and Cmd.decode_float(cmd[13:19]) == -0.25


@pytest.mark.xfail(reason = 'decode_command() does not skip the 4-char jog '
                   'id before parsing axes; debug decoder only')
def test_jog_command_round_trips():
    decoded = Cmd.decode_command(Cmd.jog(0x1a2b, {'x': 0.5}))
    assert decoded['x'] == pytest.approx(0.5)


def test_dwell_command():
    cmd = Cmd.dwell(1.5)
    assert cmd[0] == Cmd.DWELL
    assert Cmd.decode_float(cmd[1:]) == 1.5


# --- decoder ----------------------------------------------------------------

def test_decode_set_int_and_float():
    assert Cmd.decode_command('$0me=1') == {
        'type': 'set', 'name': '0me', 'value': 1}

    decoded = Cmd.decode_command('#fo=1.5')
    assert decoded == {
        'type': 'set', 'sync': True, 'name': 'fo', 'value': 1.5}


@pytest.mark.xfail(reason = 'decode_command() assigns bool to a local '
                   'instead of data["value"]; debug decoder only')
def test_decode_set_bool():
    assert Cmd.decode_command('$sr=true')['value'] is True


def test_decode_simple_commands():
    for code, type in (('P', 'pause'), ('U', 'unpause'), ('E', 'estop'),
                       ('C', 'clear'), ('F', 'flush'), ('c', 'resume')):
        assert Cmd.decode_command(code)['type'] == type


def test_decode_empty_command_is_none():
    assert Cmd.decode_command('') is None


def test_multiline_decode_yields_each_command():
    cmd = Cmd.line({'x': 1.0}, 0, 0, 0, [0] * 7, [(1.0, 100.0)])
    types = [d['type'] for d in Cmd.decode(cmd)]
    assert types == ['line', 'speed']
