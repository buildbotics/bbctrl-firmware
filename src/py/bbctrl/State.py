import logging
import bbctrl


log = logging.getLogger('State')


machine_state_vars = 'xp yp zp ap bp cp t fo so'.split()


class State(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.vars = {}
        self.callbacks = {}
        self.changes = {}
        self.next_id = 1
        self.listeners = {}
        self.timeout = None
        self.machine_vars = {}
        self.machine_var_set = set()
        self.machine_cmds = {}

        for i in range(4):
            # Add home direction callbacks
            self.set_callback(str(i) + 'hd',
                     lambda name, i = i: self.motor_home_direction(i))

            # Add home position callbacks
            self.set_callback(str(i) + 'hp',
                     lambda name, i = i: self.motor_home_position(i))

            # Set not homed
            self.set('%dhomed' % i, False)


    def _notify(self):
        if not self.changes: return

        for listener in self.listeners.values():
            try:
                listener(self.changes)

            except Exception as e:
                log.error('Updating listener: %s', traceback.format_exc())

        self.changes = {}
        self.timeout = None


    def resolve(self, name):
        # Resolve axis prefixes to motor numbers
        if 2 < len(name) and name[1] == '_' and name[0] in 'xyzabc':
            motor = self.find_motor(name[0])
            if motor is not None: return str(motor) + name[2:]

        return name


    def has(self, name): return self.resolve(name) in self.vars
    def set_callback(self, name, cb): self.callbacks[self.resolve(name)] = cb


    def set(self, name, value):
        name = self.resolve(name)

        if not name in self.vars or self.vars[name] != value:
            self.vars[name] = value
            self.changes[name] = value

            # Trigger listener notify
            if self.timeout is None:
                self.timeout = self.ctrl.ioloop.call_later(0.25, self._notify)


    def update(self, update):
        for name, value in update.items():
            self.set(name, value)


    def get(self, name, default = None):
        name = self.resolve(name)

        if name in self.vars: return self.vars[name]
        if name in self.callbacks: return self.callbacks[name](name)
        if default is None: log.error('State variable "%s" not found' % name)
        return default


    def config(self, code, value):
        if code in self.machine_var_set: self.ctrl.avr.set(code, value)
        else: self.set(code, value)


    def add_listener(self, listener):
        sid = self.next_id
        self.next_id += 1

        self.listeners[sid] = listener
        if self.vars: listener(self.vars)

        return sid


    def remove_listener(self, sid): del self.listeners[sid]


    def machine_cmds_and_vars(self, data):
        self.machine_cmds = data['commands']
        self.machine_vars = data['variables']

        # Record all machine vars, indexed or not
        self.machine_var_set = set()
        for code, spec in self.machine_vars.items():
            if 'index' in spec:
                for index in spec['index']:
                    self.machine_var_set.add(index + code)
            else: self.machine_var_set.add(code)

        self.ctrl.config.reload() # Indirectly configures AVR
        self.restore_machine_state()


    def restore_machine_state(self):
        for name in machine_state_vars:
            if name in self.vars:
                value = self.vars[name]
                if isinstance(value, str): value = '"' + value + '"'
                if isinstance(value, bool): value = int(value)

                self.ctrl.avr.set(name, value)


    def find_motor(self, axis):
        for motor in range(6):
            if not ('%dan' % motor) in self.vars: continue
            motor_axis = 'xyzabc'[self.vars['%dan' % motor]]
            if motor_axis == axis.lower() and self.vars['%dpm' % motor]:
                return motor


    def is_axis_homed(self, axis):
        return self.get('%s_homed' % axis, False)


    def axis_can_home(self, axis):
        motor = self.find_motor(axis)
        if motor is None: return False
        if not self.motor_enabled(motor): return False

        homing_mode = self.motor_homing_mode(motor)
        if homing_mode == 1: return bool(int(self.get(axis + '_ls'))) # min sw
        if homing_mode == 2: return bool(int(self.get(axis + '_xs'))) # max sw
        return False


    def motor_enabled(self, motor): return bool(int(self.vars['%dpm' % motor]))
    def motor_homing_mode(self, motor): return int(self.vars['%dho' % motor])


    def motor_home_direction(self, motor):
        homing_mode = self.motor_homing_mode(motor)
        if homing_mode == 1: return -1 # Switch min
        if homing_mode == 2: return 1  # Switch max
        return 0 # Disabled


    def motor_home_position(self, motor):
        homing_mode = self.motor_homing_mode(motor)
        if homing_mode == 1: return self.vars['%dtn' % motor] # Min soft limit
        if homing_mode == 2: return self.vars['%dtm' % motor] # Max soft limit
        return 0 # Disabled
