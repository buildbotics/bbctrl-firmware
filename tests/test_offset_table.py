'''Tests for bbctrl.OffsetTable: work offsets and tool lengths that survive a
planner reset by being saved to disk and loaded into each new planner.

The planner is faked at its seam (set_coord_offset, set_tool_offset and the
get_offsets() result).  The tests at the end drive the real CAMotics module,
when it is importable, to check the contract the fake stands in for.
'''

import json
import math

import pytest

from bbctrl.OffsetTable import OffsetTable, WORK_SYSTEMS, WORK_AXES


class FakeLog:
    def __init__(self): self.warnings, self.errors = [], []
    def warning(self, msg): self.warnings.append(msg)
    def error(self, msg): self.errors.append(msg)


class FakePlanner:
    def __init__(self): self.coord, self.tools = {}, {}

    def set_coord_offset(self, cs, axis, mm): self.coord[(cs, axis)] = mm
    def set_tool_offset(self, tool, axis, mm): self.tools[(tool, axis)] = mm


@pytest.fixture
def log(): return FakeLog()


@pytest.fixture
def path(tmp_path): return str(tmp_path / 'offsets.json')


def table(path, log): return OffsetTable(path, log)


def planner_offsets(coord_systems = None, tools = None):
    return {'coord_systems': coord_systems or {}, 'tools': tools or {}}


# Defaults and validation

def test_a_new_table_is_all_zero(path, log):
    t = table(path, log)

    assert all(t.get_work(cs, axis) == 0
               for cs in WORK_SYSTEMS for axis in WORK_AXES)
    assert all(t.get_tool_length(n) == 0 for n in range(1, 100))
    assert t.tool_reference == 0


def test_tables_cover_g54_to_g59_and_t1_to_t99(path, log):
    t = table(path, log)

    assert WORK_SYSTEMS == ('G54', 'G55', 'G56', 'G57', 'G58', 'G59')
    for bad in (0, 100):
        with pytest.raises(ValueError): t.set_tool_length(bad, 1)
    with pytest.raises(KeyError): t.set_work('G59.1', 'x', 1)
    with pytest.raises(ValueError): t.set_work('G54', 'u', 1)


# Save and load

def test_save_then_load_restores_every_value(path, log):
    t = table(path, log)
    t.set_work('G54', 'z', -123.456)
    t.set_work('G59', 'a', 90)
    t.set_tool_length(1, 42.5)
    t.set_tool_length(99, 101.25)
    t.set_tool_reference(-210.5)
    t.save()

    loaded = table(path, log)
    loaded.load()

    assert loaded.to_dict() == t.to_dict()
    assert loaded.get_work('G54', 'z') == -123.456
    assert loaded.get_work('G59', 'a') == 90
    assert loaded.get_tool_length(99) == 101.25
    assert loaded.tool_reference == -210.5
    assert log.warnings == [] and log.errors == []


def test_saving_leaves_no_temporary_file(path, log, tmp_path):
    table(path, log).save()
    assert sorted(p.name for p in tmp_path.iterdir()) == ['offsets.json']


def test_a_missing_file_loads_as_zero_and_is_not_created(path, log, tmp_path):
    t = table(path, log)
    t.load()

    assert t.to_dict() == table(path, log).to_dict()
    assert list(tmp_path.iterdir()) == []


def test_missing_entries_load_as_zero(path, log):
    with open(path, 'w') as f:
        json.dump({'work': {'G55': {'x': 5}}, 'tools': {'7': {'z': 3}}}, f)

    t = table(path, log)
    t.load()

    assert t.get_work('G55', 'x') == 5
    assert t.get_work('G55', 'y') == 0
    assert t.get_work('G54', 'x') == 0
    assert t.get_tool_length(7) == 3
    assert t.get_tool_length(8) == 0
    assert t.tool_reference == 0


@pytest.mark.parametrize('bad', ['12', None, True, float('nan'),
                                 float('inf'), [1]])
def test_an_invalid_value_loads_as_zero_with_a_warning(path, log, bad):
    data = table(path, log).to_dict()
    data['work']['G54']['z'] = bad
    data['work']['G54']['x'] = 7
    with open(path, 'w') as f: f.write(json.dumps(data))

    t = table(path, log)
    t.load()

    assert t.get_work('G54', 'z') == 0
    assert t.get_work('G54', 'x') == 7
    assert len(log.warnings) == 1 and 'G54z' in log.warnings[0]


@pytest.mark.parametrize('content', ['{not json', '[1, 2]', ''])
def test_an_unreadable_file_is_set_aside_not_overwritten(path, log, content):
    with open(path, 'w') as f: f.write(content)

    t = table(path, log)
    t.load()

    assert t.get_work('G54', 'z') == 0
    assert len(log.errors) == 1
    with open(path + '.bad') as f: assert f.read() == content


# Edits from the web UI

def test_an_edit_changes_only_the_values_given(path, log):
    t = table(path, log)
    t.set_work('G54', 'x', 1)

    t.edit({'work': {'G55': {'z': -12.5}}})

    assert t.get_work('G55', 'z') == -12.5
    assert t.get_work('G54', 'x') == 1


def test_an_edit_can_set_work_tools_and_reference_together(path, log):
    t = table(path, log)

    t.edit({'work': {'G59': {'a': 90, 'x': 1.5}},
            'tools': {'1': {'z': 10}, '99': {'z': 80.25}},
            'tool_reference': {'z': -200}})

    assert t.get_work('G59', 'a') == 90
    assert t.get_work('G59', 'x') == 1.5
    assert t.get_tool_length(1) == 10
    assert t.get_tool_length(99) == 80.25
    assert t.tool_reference == -200


@pytest.mark.parametrize('changes', [
    {},
    [],
    {'offsets': {}},
    {'work': {'G59.1': {'x': 1}}},
    {'work': {'G54': {'u': 1}}},
    {'work': {'G54': {'xy': 1}}},
    {'work': {'G54': 5}},
    {'work': {'G54': {'x': '5'}}},
    {'work': {'G54': {'x': float('nan')}}},
    {'work': {'G54': {'x': True}}},
    {'tools': {'0': {'z': 1}}},
    {'tools': {'100': {'z': 1}}},
    {'tools': {'5': {'x': 1}}},
    {'tool_reference': {'x': 1}},
    {'tool_reference': {'z': None}},
    {'tool_reference': 5},
])
def test_an_invalid_edit_is_refused(path, log, changes):
    with pytest.raises(ValueError): table(path, log).edit(changes)


def test_a_partly_invalid_edit_changes_nothing(path, log):
    t = table(path, log)
    before = t.to_dict()

    with pytest.raises(ValueError):
        t.edit({'work': {'G54': {'x': 5}}, 'tools': {'5': {'z': 'long'}}})

    assert t.to_dict() == before


# Loading into the planner

def test_apply_loads_every_work_offset_and_tool_length(path, log):
    t = table(path, log)
    t.set_work('G54', 'x', 1.5)
    t.set_work('G59', 'z', -60)
    t.set_tool_length(5, 42)
    t.set_tool_reference(-200)
    planner = FakePlanner()

    t.apply_to(planner)

    assert len(planner.coord) == 6 * 6
    assert planner.coord[(1, 'x')] == 1.5  # G54 is CAMotics system 1
    assert planner.coord[(6, 'z')] == -60  # G59 is CAMotics system 6
    assert planner.coord[(3, 'y')] == 0
    assert sorted(planner.tools) == [(n, 'z') for n in range(1, 100)]
    assert planner.tools[(5, 'z')] == 42
    # The reference is a datum for measuring tools, never a G43 offset
    assert -200 not in planner.tools.values()


# Reading back from the planner

def test_an_empty_planner_result_changes_nothing(path, log):
    t = table(path, log)
    t.set_work('G54', 'z', 3)

    assert t.update_from({}) is False
    assert t.update_from(planner_offsets()) is False
    assert t.get_work('G54', 'z') == 3


def test_one_changed_work_offset_is_copied(path, log):
    t = table(path, log)

    assert t.update_from(planner_offsets({'2': {'y': 7.0}})) is True
    assert t.get_work('G55', 'y') == 7
    assert t.get_work('G55', 'x') == 0


def test_identical_values_report_no_change(path, log):
    t = table(path, log)
    t.set_work('G54', 'z', -50)
    t.set_tool_length(3, 12)

    offsets = planner_offsets({'1': {'z': -50.0}}, {'3': {'z': 12.0}})
    assert t.update_from(offsets) is False


def test_many_changes_are_copied_together(path, log):
    t = table(path, log)
    coord = {str(cs): {axis: cs * 10.0 + i for i, axis in enumerate('xyz')}
             for cs in range(1, 7)}
    tools = {str(n): {'z': n / 2} for n in range(1, 100)}

    assert t.update_from(planner_offsets(coord, tools)) is True
    assert t.get_work('G56', 'y') == 31
    assert t.get_work('G59', 'z') == 62
    assert t.get_tool_length(99) == 49.5


def test_entries_outside_the_table_are_ignored(path, log):
    t = table(path, log)
    before = t.to_dict()

    offsets = planner_offsets(
        {'7': {'x': 1.0}, '9': {'z': 2.0}},          # G59.1 to G59.3
        {'0': {'z': 3.0}, '100': {'z': 4.0},         # T0 is not a tool
         '5': {'x': 5.0, 'y': 6.0}})                 # Only lengths are kept

    assert t.update_from(offsets) is False
    assert t.to_dict() == before


# The real planner, when available

def _camotics():
    try: from bbctrl import camotics
    except ImportError: camotics = pytest.importorskip('camotics')
    if not hasattr(camotics.Planner(), 'get_offsets'):
        pytest.skip('camotics built without offset support')
    return camotics


def _run(planner, gcode):
    'Run G-code to the end and return the named variables it set.'
    planner.load_string(gcode, {'default-units': 'METRIC'})
    names = {}
    while planner.has_more():
        block = planner.next()
        if block['type'] == 'set': names[block['name']] = block['value']
    return names


def test_camotics_round_trip_through_a_new_planner(path, log):
    camotics = _camotics()
    t = table(path, log)
    t.set_work('G55', 'z', -42.5)
    t.set_tool_length(7, 30)

    planner = camotics.Planner()
    t.apply_to(planner)
    reloaded = table(path, log)
    reloaded.update_from(planner.get_offsets())

    assert reloaded.to_dict() == t.to_dict()


def test_camotics_g10_reports_a_change_in_millimeters(path, log):
    camotics = _camotics()
    planner = camotics.Planner()
    table(path, log).apply_to(planner)

    names = _run(planner, 'G20 G10 L2 P2 X1\nG10 L1 P4 Z2\nG21\n')
    t = table(path, log)

    assert '_offset_table_id' in names
    assert t.update_from(planner.get_offsets()) is True
    assert t.get_work('G55', 'x') == pytest.approx(25.4)
    assert t.get_tool_length(4) == pytest.approx(50.8)


def test_camotics_loading_the_table_reports_no_change(path, log):
    camotics = _camotics()
    planner = camotics.Planner()
    t = table(path, log)
    t.set_work('G54', 'x', 5)
    t.apply_to(planner)

    assert not planner.has_more()
    assert '_offset_table_id' not in _run(planner, 'G0 X0\n')


def test_camotics_tool_length_moves_the_reported_offset(path, log):
    camotics = _camotics()
    planner = camotics.Planner()
    t = table(path, log)
    t.set_work('G54', 'z', -100)
    t.set_tool_length(2, 25)
    t.apply_to(planner)

    assert _run(planner, 'G43 H2\n')['_offset_z'] == pytest.approx(-75)
    assert _run(planner, 'G49\n')['_offset_z'] == pytest.approx(-100)
