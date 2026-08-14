'''Tests for the AVR serial protocol layer in bbctrl.Comm.

Comm frames newline-delimited JSON coming from the AVR and streams
newline-terminated commands to it.  The serial port and controller are faked
at their seams (avr read/write handlers, ioloop, state); the framing, queueing
and dispatch logic under test is real.
'''

import pytest

from bbctrl.Comm import Comm, driver_flags_to_string


class FakeLog:
    def __init__(self): self.messages = []

    def get(self, name): return self

    def _log(self, level, msg, *args, **kwargs):
        if args: msg = msg % args
        self.messages.append((level, msg))

    def info(self, msg, *args, **kwargs): self._log('info', msg, *args)
    def debug(self, msg, *args, **kwargs): self._log('debug', msg, *args)
    def warning(self, msg, *args, **kwargs): self._log('warning', msg, *args)
    def error(self, msg, *args, **kwargs): self._log('error', msg, *args)

    def of_level(self, level):
        return [m for l, m in self.messages if l == level]


class FakeIOLoop:
    def __init__(self): self.calls = []
    def call_later(self, delay, cb, *args): self.calls.append((delay, cb))


class FakeState:
    def __init__(self):
        self.updates = []
        self.machine_vars = None

    def update(self, update): self.updates.append(update)
    def set_machine_vars(self, vars): self.machine_vars = vars
    def get(self, name, default = None): return default


class FakeCtrl:
    def __init__(self):
        self.log = FakeLog()
        self.ioloop = FakeIOLoop()
        self.state = FakeState()
        self.ready_called = False

    def ready(self): self.ready_called = True
    def configure(self): pass


class FakeAvr:
    def __init__(self):
        self.read_cb = self.write_cb = None
        self.write_enabled = None
        self.i2c_commands = []
        self.flushed = False

    def set_handlers(self, read, write):
        self.read_cb, self.write_cb = read, write

    def enable_write(self, enable): self.write_enabled = enable
    def i2c_command(self, cmd, byte, word, block):
        self.i2c_commands.append(cmd)
    def flush_output(self): self.flushed = True


class TestableComm(Comm):
    def __init__(self, ctrl, avr):
        self.next_commands = []
        self.errors = 0
        self.results = []
        super().__init__(ctrl, avr)

    def comm_next(self):
        return self.next_commands.pop(0) if self.next_commands else None

    def comm_error(self): self.errors += 1
    def comm_result(self, result): self.results.append(result)


@pytest.fixture
def comm():
    ctrl, avr = FakeCtrl(), FakeAvr()
    return TestableComm(ctrl, avr), ctrl, avr


# --- incoming framing -------------------------------------------------------

def test_state_update_split_across_reads_is_reassembled(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"xx":"READY"')
    assert ctrl.state.updates == []   # incomplete line buffered

    avr.read_cb(b',"v":0}\n')
    assert ctrl.state.updates == [{'xx': 'READY', 'v': 0}]


def test_multiple_lines_in_one_read_are_all_dispatched(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"1en":100}\n{"2en":-3}\n')
    assert ctrl.state.updates == [{'1en': 100}, {'2en': -3}]


def test_blank_lines_are_ignored(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'\n\n{"v":1}\n\n')
    assert ctrl.state.updates == [{'v': 1}]


def test_bad_json_is_logged_and_skipped(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"broken\n{"v":2}\n')

    assert ctrl.state.updates == [{'v': 2}]   # next line still processed
    assert any('broken' in m for m in c.log.of_level('warning'))


def test_state_change_marks_controller_ready(comm):
    c, ctrl, avr = comm
    assert not ctrl.ready_called

    avr.read_cb(b'{"xx":"HOLDING"}\n')
    assert ctrl.ready_called


def test_result_messages_reach_comm_result(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"result":42,"id":7}\n')
    assert c.results == [42]


def test_error_log_messages_trigger_comm_error(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"msg":"boom","level":"error"}\n')
    assert c.errors == 1
    assert 'boom' in c.log.of_level('error')


def test_variables_message_triggers_reload_sequence(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"variables":{"vm":{"type":"f32"}}}\n')

    assert ctrl.state.machine_vars == {'vm': {'type': 'f32'}}
    assert 'D' in c.queue          # var dump requested
    # One set-axis command per axis restores positions
    assert sum(1 for q in c.queue if q.startswith('a')) == 6


# --- estop gating -----------------------------------------------------------

def test_estop_drops_messages_until_firmware_reboots(comm):
    c, ctrl, avr = comm

    c.enter_estop()
    avr.read_cb(b'{"v":5}\n')
    assert ctrl.state.updates == []   # dropped while estopped

    avr.read_cb(b'{"firmware":"x"}\n')
    assert not c.estopped
    # Reboot triggers reconnect: resume + help queued
    assert 'c' in c.queue and 'h' in c.queue


def test_estop_clears_pending_commands(comm):
    c, ctrl, avr = comm

    c.queue_command('D')
    c.estop()

    assert len(c.queue) == 0
    assert avr.flushed
    assert 'E' in avr.i2c_commands


# --- outgoing command stream ------------------------------------------------

def drain(c, avr, writes):
    '''Drive Comm._write like the serial layer would, `writes` bytes at a
    time, until it disables writing.  Returns everything written.'''
    sent = b''

    def write_cb(data):
        nonlocal sent
        n = min(writes, len(data))
        sent += data[:n]
        return n

    for _ in range(100):
        avr.write_cb(write_cb)
        if avr.write_enabled is False: return sent

    raise AssertionError('write never completed')


def test_commands_are_newline_terminated(comm):
    c, ctrl, avr = comm

    c.queue_command('D')
    assert drain(c, avr, 100) == b'D\n'


def test_queued_commands_are_sent_in_order(comm):
    c, ctrl, avr = comm

    c.queue_command('D')
    c.queue_command('c')
    assert drain(c, avr, 100) == b'D\nc\n'


def test_partial_writes_resume_where_they_left_off(comm):
    c, ctrl, avr = comm

    c.queue_command('$0me=1')
    assert drain(c, avr, 1) == b'$0me=1\n'   # one byte at a time


def test_planner_commands_flow_after_queue_empties(comm):
    c, ctrl, avr = comm

    c.queue_command('D')
    c.next_commands = ['c']
    assert drain(c, avr, 100) == b'D\nc\n'


def test_write_disables_when_nothing_to_send(comm):
    c, ctrl, avr = comm

    avr.write_cb(lambda data: len(data))
    assert avr.write_enabled is False


# --- driver flags -----------------------------------------------------------

def test_driver_flags_to_string_maps_bits():
    assert driver_flags_to_string(1 << 0) == 'over temp'
    assert driver_flags_to_string((1 << 0) | (1 << 2)) == \
        'over temp, over current b'
    assert driver_flags_to_string(1 << 8) == 'comm error'
    assert driver_flags_to_string(0) == ''


def test_motor_flag_changes_are_logged(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"1df":1}\n')
    assert any('Motor 1 flags: over temp' in m for m in c.log.of_level('info'))


@pytest.mark.xfail(reason = 'Comm._log_motor_flags() iterates range(3); '
                   'motor 3 driver faults are never logged')
def test_motor_3_flag_changes_are_logged(comm):
    c, ctrl, avr = comm

    avr.read_cb(b'{"3df":1}\n')
    assert any('Motor 3 flags: over temp' in m for m in c.log.of_level('info'))
