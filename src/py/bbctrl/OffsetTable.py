################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
#                                                                              #
################################################################################

'''
Persistent work offsets and tool length table.

The CAMotics planner holds the live G54-G59 offsets and tool table, but a new
planner is built on every reset (startup, E-stop, serial reconnect), which
used to lose them.  This table is the copy that survives: it is loaded into
each new planner and updated whenever G10 or a parameter assignment changes
the planner's copy.

All values are millimeters.

Tool lengths are measured from the spindle nose, the gauge line, so a tool's
G43 offset is its length alone.  The tool reference (T0) is not a tool: it is
where the spindle nose itself sits, typically the machine Z at which it
triggers the tool setter.  It lets tools be measured against one fixed datum,
or offline, instead of against each other.  T99 is the probe.
'''

import json
import math
import os

__all__ = ['OffsetTable']


WORK_SYSTEMS = ('G54', 'G55', 'G56', 'G57', 'G58', 'G59')
WORK_AXES    = 'xyzabc'
TOOL_NUMBERS = range(1, 100)
VERSION      = 1


def _cs_number(system: str) -> int:
    'CAMotics numbers coordinate systems from 1 (G54).'
    return WORK_SYSTEMS.index(system) + 1


def _to_mm(value, what: str, log) -> float:
    if (isinstance(value, (int, float)) and not isinstance(value, bool) and
        math.isfinite(value)): return float(value)

    log.warning('Ignoring invalid offset %s=%r, using 0' % (what, value))
    return 0.0


class OffsetTable:
    def __init__(self, path: str, log) -> None:
        self.path = path
        self.log  = log
        self.work = {cs: {axis: 0.0 for axis in WORK_AXES}
                     for cs in WORK_SYSTEMS}
        self.tools = {n: 0.0 for n in TOOL_NUMBERS}
        self.tool_reference = 0.0


    def get_work(self, system: str, axis: str) -> float:
        return self.work[system][axis]


    def set_work(self, system: str, axis: str, mm: float) -> None:
        if axis not in WORK_AXES: raise ValueError('Invalid axis %r' % axis)
        self.work[system][axis] = float(mm)


    def get_tool_length(self, tool: int) -> float: return self.tools[tool]


    def set_tool_length(self, tool: int, mm: float) -> None:
        if tool not in TOOL_NUMBERS: raise ValueError('Invalid tool %r' % tool)
        self.tools[tool] = float(mm)


    def set_tool_reference(self, mm: float) -> None:
        self.tool_reference = float(mm)


    def to_dict(self) -> dict:
        return {
            'version':        VERSION,
            'work':           {cs: dict(axes) for cs, axes in self.work.items()},
            'tool_reference': {'z': self.tool_reference},
            'tools':          {str(n): {'z': z} for n, z in self.tools.items()},
        }


    def from_dict(self, data: dict) -> None:
        'Fill the table from saved data.  Missing entries stay zero.'
        log = self.log

        work = data.get('work', {})
        for cs in WORK_SYSTEMS:
            for axis in WORK_AXES:
                value = work.get(cs, {}).get(axis, 0)
                self.work[cs][axis] = _to_mm(value, cs + axis, log)

        tools = data.get('tools', {})
        for n in TOOL_NUMBERS:
            value = tools.get(str(n), {}).get('z', 0)
            self.tools[n] = _to_mm(value, 'T%d' % n, log)

        value = data.get('tool_reference', {}).get('z', 0)
        self.tool_reference = _to_mm(value, 'T0', log)


    def load(self) -> None:
        if not os.path.exists(self.path): return

        try:
            with open(self.path, 'r') as f: data = json.load(f)
            if not isinstance(data, dict): raise ValueError('Not an object')

        except Exception as e:
            # Keep the unreadable file for inspection rather than overwrite it
            bad = self.path + '.bad'
            os.replace(self.path, bad)
            self.log.error('Could not read offsets, all offsets are zero.  '
                           'The file was moved to %s: %s' % (bad, e))
            return

        self.from_dict(data)


    def save(self) -> None:
        # Write then rename, so power loss leaves the old or the new table
        tmp = self.path + '.tmp'
        with open(tmp, 'w') as f:
            json.dump(self.to_dict(), f, indent = 2)
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, self.path)


    def apply_to(self, planner) -> None:
        'Load the table into a CAMotics planner.'
        for cs in WORK_SYSTEMS:
            for axis in WORK_AXES:
                planner.set_coord_offset(_cs_number(cs), axis,
                                         self.work[cs][axis])

        for n in TOOL_NUMBERS:
            planner.set_tool_offset(n, 'z', self.tools[n])


    def update_from(self, offsets: dict) -> bool:
        '''Copy values from the planner's get_offsets() result.

        Returns True if anything changed.  Coordinate systems past G59, tool
        0, tools past T99 and axes other than tool Z are not kept.
        '''
        changed = False
        coord_systems = offsets.get('coord_systems', {})

        for cs in WORK_SYSTEMS:
            axes = coord_systems.get(str(_cs_number(cs)), {})

            for axis in WORK_AXES:
                if axis in axes and axes[axis] != self.work[cs][axis]:
                    self.work[cs][axis] = float(axes[axis])
                    changed = True

        tools = offsets.get('tools', {})
        for n in TOOL_NUMBERS:
            z = tools.get(str(n), {}).get('z')
            if z is not None and z != self.tools[n]:
                self.tools[n] = float(z)
                changed = True

        return changed
