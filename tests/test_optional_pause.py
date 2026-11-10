'''Tests for M01 Break (optional pause).

An M01 in a program reaches the AVR as an optional pause, which holds the
machine with the reason "Optional pause".  The controller resumes those holds
by itself unless M01 Break is on.  M01 Break is a persistent toggle: it stays
on through resumes until it is turned off.
'''

from bbctrl.Mach import Mach


class FakeState:
    '''Just the get/set part of State, with no controller behind it.'''
    def __init__(self, **vars): self.vars = dict(vars)
    def get(self, name, default = None): return self.vars.get(name, default)
    def set(self, name, value): self.vars[name] = value


class FakeCtrl:
    def __init__(self, state): self.state = state


def held_at_m01(m01_break):
    '''A Mach holding at an M01, recording whether it resumed.'''
    mach = Mach.__new__(Mach) # No AVR, planner or I/O behind it
    mach.ctrl = FakeCtrl(FakeState(xx = 'HOLDING', pr = 'Optional pause',
                                   optional_pause = m01_break))
    mach.unpausing = False
    mach.stopping = False
    mach.resumed = 0

    def _unpause(): mach.resumed += 1
    mach._unpause = _unpause

    return mach


def test_m01_resumes_by_itself_when_m01_break_is_off():
    mach = held_at_m01(m01_break = False)
    mach._update({'pr': 'Optional pause'})
    assert mach.resumed == 1


def test_m01_holds_when_m01_break_is_on():
    mach = held_at_m01(m01_break = True)
    mach._update({'pr': 'Optional pause'})
    assert mach.resumed == 0


def test_resuming_from_m01_leaves_m01_break_on():
    mach = held_at_m01(m01_break = True)
    mach.unpause()
    assert mach.resumed == 1
    assert mach.ctrl.state.get('optional_pause') is True


def test_m01_break_turns_off():
    mach = held_at_m01(m01_break = True)
    mach.optional_pause(False)
    assert mach.ctrl.state.get('optional_pause') is False


def test_m01_break_turns_on_by_default():
    mach = held_at_m01(m01_break = False)
    mach.optional_pause()
    assert mach.ctrl.state.get('optional_pause') is True
