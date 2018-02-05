import json
import logging
import camotics.gplan as gplan
import bbctrl.Cmd as Cmd

log = logging.getLogger('Planner')



class Planner():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.lastID = -1
        self.done = False

        ctrl.state.add_listener(lambda x: self.update(x))

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

        return {
            "start":     start,
            "max-vel":   get_vector('vm', 1000),
            "max-accel": get_vector('am', 1000),
            "max-jerk":  get_vector('jm', 1000000),
            # TODO junction deviation & accel
            }


    def update(self, update):
        if 'id' in update:
            id = update['id']
            if id: self.planner.release(id - 1)

        if update.get('x', '') == 'HOLDING' and \
                self.ctrl.state.get('pr', '') == 'Switch found':
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
        self.done = False


    def get_var(self, name):
        value = 0
        if len(name) and name[0] == '_':
            value = self.ctrl.state.get(name[1:], 0)

        log.info('Get: %s=%s' % (name, value))
        return value


    def log(self, line):
        line = line.strip()
        if len(line) < 3: return

        if line[0] == 'I': log.info(line[3:])
        if line[0] == 'D': log.debug(line[3:])
        if line[0] == 'W': log.warning(line[3:])
        if line[0] == 'E': log.error(line[3:])
        if line[0] == 'C': log.critical(line[3:])


    def mdi(self, cmd):
        self.planner.set_config(self.get_config())
        self.planner.mdi(cmd)
        self.done = False


    def load(self, path):
        self.planner.set_config(self.get_config())
        self.planner.load('upload' + path)
        self.done = False


    def reset(self):
        self.planner = gplan.Planner(self.get_config())
        self.planner.set_resolver(self.get_var)
        self.planner.set_logger(self.log)


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

        if not self.done:
            self.done = True

            # Cause last cmd to flush when complete
            if 0 <= self.lastID:
                return '#id=%d' % (self.lastID + 1)
