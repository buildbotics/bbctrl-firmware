from inevent.Constants import *


def axes_to_string(axes):
    return "({:6.3f}, {:6.3f}, {:6.3f}, {:6.3f})".format(*axes)


def print_event(event, state):
    print("{} {}: ".format(event.get_source(), event.get_type_name()), end = '')

    if event.type == EV_ABS:
        print(axes_to_string(state.get_joystick3d()) + " " +
              axes_to_string(state.get_joystickR3d()) + " " +
              "({:2.0f}, {:2.0f}) ".format(*state.get_hat()))

    if event.type == EV_REL:
        print("({:d}, {:d}) ".format(*state.get_mouse()) +
              "({:d}, {:d})".format(*state.get_wheel()))

    if event.type == EV_KEY:
        state = "pressed" if event.value else "released"
        print("0x{:x} {}".format(event.code, state))


class JogHandler:
    def __init__(self, config):
        self.config = config
        self.axes = [0.0, 0.0, 0.0, 0.0]
        self.speed = 3
        self.activate = 0


    def changed(self):
        print(axes_to_string(self.axes) + " x {:d}".format(self.speed))


    def __call__(self, event, state):
        if event.type not in [EV_ABS, EV_REL, EV_KEY]: return

        changed = False

        # Process event
        if event.type == EV_ABS and event.code in self.config['axes']:
            pass

        elif event.type == EV_ABS and event.code in self.config['arrows']:
            axis = self.config['arrows'].index(event.code)

            if event.value < 0:
                if axis == 1: print('up')
                else: print('left')

            elif 0 < event.value:
                if axis == 1: print('down')
                else: print('right')

        elif event.type == EV_KEY and event.code in self.config['speed']:
            old_speed = self.speed
            self.speed = self.config['speed'].index(event.code) + 1
            if self.speed != old_speed: changed = True

        elif event.type == EV_KEY and event.code in self.config['activate']:
            index = self.config['activate'].index(event.code)

            if event.value: self.activate |= 1 << index
            else: self.activate &= ~(1 << index)

        if self.config.get('verbose', False): print_event(event, state)

        # Update axes
        old_axes = list(self.axes)

        for axis in range(4):
            self.axes[axis] = event.stream.state.abs[self.config['axes'][axis]]
            if abs(self.axes[axis]) < self.config['deadband']:
                self.axes[axis] = 0
            if not (1 << axis) & self.activate and self.activate:
                self.axes[axis] = 0

        if old_axes != self.axes: changed = True

        if changed: self.changed()
