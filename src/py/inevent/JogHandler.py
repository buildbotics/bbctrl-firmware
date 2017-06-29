import logging

from inevent.Constants import *


log = logging.getLogger('inevent')


def axes_to_string(axes):
    return '({:6.3f}, {:6.3f}, {:6.3f})'.format(*axes)


def event_to_string(event, state):
    s = '{} {}: '.format(event.get_source(), event.get_type_name())

    if event.type == EV_ABS:
        s += axes_to_string(state.get_joystick3d()) + ' ' + \
            axes_to_string(state.get_joystickR3d()) + ' ' + \
            '({:2.0f}, {:2.0f}) '.format(*state.get_hat())

    if event.type == EV_REL:
        s += '({:d}, {:d}) '.format(*state.get_mouse()) + \
            '({:d}, {:d})'.format(*state.get_wheel())

    if event.type == EV_KEY:
        state = 'pressed' if event.value else 'released'
        s += '0x{:x} {}'.format(event.code, state)

    return s


class JogHandler:
    def __init__(self, config):
        self.config = config
        self.axes = [0.0, 0.0, 0.0, 0.0]
        self.speed = 3
        self.activate = 0


    def changed(self):
        log.debug(axes_to_string(self.axes) + ' x {:d}'.format(self.speed))


    def up(self): log.debug('up')
    def down(self): log.debug('down')
    def left(self): log.debug('left')
    def right(self): log.debug('right')


    def __call__(self, event, state):
        if event.type not in [EV_ABS, EV_REL, EV_KEY]: return

        changed = False

        # Process event
        if event.type == EV_ABS and event.code in self.config['axes']:
            pass

        elif event.type == EV_ABS and event.code in self.config['arrows']:
            axis = self.config['arrows'].index(event.code)

            if event.value < 0:
                if axis == 1: self.up()
                else: self.left()

            elif 0 < event.value:
                if axis == 1: self.down()
                else: self.right()

        elif event.type == EV_KEY and event.code in self.config['speed']:
            old_speed = self.speed
            self.speed = self.config['speed'].index(event.code) + 1
            if self.speed != old_speed: changed = True

        elif event.type == EV_KEY and event.code in self.config['activate']:
            index = self.config['activate'].index(event.code)

            if event.value: self.activate |= 1 << index
            else: self.activate &= ~(1 << index)

        log.debug(event_to_string(event, state))

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
