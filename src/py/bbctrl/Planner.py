import json
import re
import logging
import camotics.gplan as gplan # pylint: disable=no-name-in-module,import-error
import bbctrl.Cmd as Cmd

log = logging.getLogger('Planner')

reLogLine = re.compile(
    r'^(?P<level>[A-Z])[0-9 ]:((?P<where>[^:]+:\d+:\d+):)?(?P<msg>.*)$')


class Planner():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.lastID = -1
        self.mode = 'idle'

        ctrl.state.add_listener(self.update)

        self.reset()


    def is_running(self): return self.planner.is_running()


    def get_config(self):
        state = self.ctrl.state

        # Axis mapping for enabled motors
        axis2motor = {}
        for i in range(3):
            if state.get('%dpm' % i, False):
                axis = 'xyzabc'[int(state.get('%dan' % i))]
                axis2motor[axis] = i

        def get_vector(name, scale = 1):
            v = {}
            for axis in 'xyzabc':
                if axis in axis2motor:
                    motor = axis2motor[axis]
                    value = state.get(str(motor) + name, None)
                    if value is not None:
                        v[axis] = value * scale
            return v

        # Starting position
        start = {}
        for axis in 'xyzabc':
            if not axis in axis2motor: continue
            value = state.get(axis + 'p', None)
            if value is not None: start[axis] = value

        config = {
            "start":     start,
            "max-vel":   get_vector('vm', 1000),
            "max-accel": get_vector('am', 1000000),
            "max-jerk":  get_vector('jm', 1000000),
            # TODO junction deviation & accel
            }

        log.info('Config:' + json.dumps(config))

        return config


    def update(self, update):
        if 'id' in update: self.planner.set_active(update['id'])

        if self.ctrl.state.get('x', '') == 'HOLDING' and \
                self.ctrl.state.get('pr', '') == 'Switch found' and \
                self.planner.is_synchronizing():
            self.ctrl.avr.unpause()


    def restart(self):
        state = self.ctrl.state
        id = state.get('id')

        position = {}
        for axis in 'xyzabc':
            if state.has(axis + 'p'):
                position[axis] = state.get(axis + 'p')

        log.info('Planner restart: %d %s' % (id, json.dumps(position)))
        self.planner.restart(id, position)


    def get_var(self, name):
        value = 0
        if len(name) and name[0] == '_':
            value = self.ctrl.state.get(name[1:], 0)

        log.info('Get: %s=%s' % (name, value))
        return value


    def log(self, line):
        line = line.strip()
        m = reLogLine.match(line)
        if not m: return

        level = m.group('level')
        msg = m.group('msg')
        where = m.group('where')

        if where is not None: extra = dict(where = where)
        else: extra = None

        if   level == 'I': log.info    (msg, extra = extra)
        elif level == 'D': log.debug   (msg, extra = extra)
        elif level == 'W': log.warning (msg, extra = extra)
        elif level == 'E': log.error   (msg, extra = extra)
        elif level == 'C': log.critical(msg, extra = extra)
        else: log.error('Could not parse planner log line: ' + line)


    def mdi(self, cmd):
        if self.mode == 'gcode':
            raise Exception('Cannot issue MDI command while GCode running')

        log.info('MDI:' + cmd)
        self.planner.load_string(cmd, self.get_config())
        self.mode = 'mdi'


    def load(self, path):
        if self.mode != 'idle':
            raise Exception('Busy, cannot start new GCode program')

        log.info('GCode:' + path)
        self.planner.load('upload' + path, self.get_config())


    def reset(self):
        self.planner = gplan.Planner()
        self.planner.set_resolver(self.get_var)
        self.planner.set_logger(self.log, 1, 'LinePlanner:3')


    def encode(self, block):
        type = block['type']

        if type == 'line':
            return Cmd.line(block['id'], block['target'], block['exit-vel'],
                            block['max-accel'], block['max-jerk'],
                            block['times'])

        if type == 'set':
            name, value = block['name'], block['value']

            if name == 'line': return Cmd.line_number(value)
            if name == 'tool': return Cmd.tool(value)
            if name == 'speed': return Cmd.speed(value)
            if name[0:1] == '_' and name[1:2] in 'xyzabc' and \
                    name[2:] == '_home':
                return Cmd.set_position(name[1], value)

            if len(name) and name[0] == '_':
                self.ctrl.state.set(name[1:], value)

            return

        if type == 'output':
            return Cmd.output(block['port'], int(float(block['value'])))

        if type == 'dwell': return Cmd.dwell(block['seconds'])
        if type == 'pause': return Cmd.pause(block['optional'])
        if type == 'seek':
            return Cmd.seek(block['switch'], block['active'], block['error'])

        raise Exception('Unknown planner type "%s"' % type)


    def has_move(self): return self.planner.has_more()


    def next(self):
        while self.planner.has_more():
            cmd = self.planner.next()
            self.lastID = cmd['id']
            cmd = self.encode(cmd)
            if cmd is not None: return cmd

        if not self.is_running(): self.mode = 'idle'
