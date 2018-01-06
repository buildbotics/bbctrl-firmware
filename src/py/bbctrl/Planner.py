import json
import logging
import camotics.gplan as gplan
import bbctrl.Cmd as Cmd

log = logging.getLogger('Planner')



class Planner():
    def __init__(self, ctrl, path):
        self.ctrl = ctrl
        self.path = path
        self.lastID = -1
        self.done = False

        vars = ctrl.avr.vars

        # Axis mapping for enabled motors
        axis2motor = {}
        for i in range(3):
            if vars.get('%dpm' % i, False):
                axis = 'xyzabc'[int(vars.get('%dan' % i))]
                axis2motor[axis] = i

        def get_vector(name, scale = 1):
            v = {}
            for axis in 'xyzabc':
                if axis in axis2motor:
                    motor = axis2motor[axis]
                    value = vars.get(str(motor) + name, None)
                    if value is not None:
                        v[axis] = value * scale
            return v

        # Starting position
        start = {}
        for axis in 'xyzabc':
            if not axis in axis2motor: continue
            value = vars.get(axis + 'p', None)
            if value is not None: start[axis] = value

        # Planner config
        self.config = {
            "start": start,
            "max-vel": get_vector('vm'),
            "max-accel": get_vector('am'),
            "max-jerk": get_vector('jm', 1000000),
            # TODO junction deviation & accel
            }
        log.info('Planner config: ' + json.dumps(self.config))

        self.reset()


    def is_running(self): return self.planner.is_running()


    def update(self, update):
        if 'id' in update:
            id = update['id']
            if id: self.planner.release(id - 1)


    def restart(self):
        vars = self.ctrl.avr.vars
        id = vars['id']

        position = {}
        for axis in 'xyzabc':
            if (axis + 'p') in vars:
                position[axis] = vars[axis + 'p']

        self.planner.restart(id, position)
        self.done = False


    def reset(self):
        self.planner = gplan.Planner(self.config)
        self.planner.load('upload' + self.path)
        self.done = False


    def encode(self, block):
        type = block['type']

        if type == 'line':
            return Cmd.line(block['id'], block['target'], block['exit-vel'],
                            block['max-accel'], block['max-jerk'],
                            block['times'])

        if type == 'ln': return Cmd.line_number(block['line'])
        if type == 'tool': return Cmd.tool(block['tool'])
        if type == 'speed': return Cmd.speed(block['speed'])
        if type == 'dwell': return Cmd.dwell(block['seconds'])
        if type == 'pause': return Cmd.pause(block['optional'])

        raise Exception('Unknown planner type "%s"' % type)


    def next(self):
        if self.planner.has_more():
            cmd = self.planner.next()
            self.lastID = cmd['id']
            return self.encode(cmd)

        if not self.done:
            self.done = True

            # Cause last cmd to flush when complete
            if 0 <= self.lastID:
                return '#id=%d' % (self.lastID + 1)
