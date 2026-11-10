'''Tests for the G-code that reselects the reported coordinate system, used to
have the planner report offsets after an edit from the Offsets page.'''

import pytest

from bbctrl.Mach import coord_system_gcode


@pytest.mark.parametrize('code, gcode', [
    (540, 'G54'), (550, 'G55'), (590, 'G59'),
    (591, 'G59.1'), (592, 'G59.2'), (593, 'G59.3'),
])
def test_reported_coordinate_system_maps_to_its_gcode(code, gcode):
    assert coord_system_gcode(code) == gcode
