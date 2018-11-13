/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

'use strict'


function is_defined(x) {return typeof x != 'undefined'}


module.exports = {
  props: ['state', 'config'],


  computed: {
    x: function () {return this._compute_axis('x')},
    y: function () {return this._compute_axis('y')},
    z: function () {return this._compute_axis('z')},
    a: function () {return this._compute_axis('a')},
    b: function () {return this._compute_axis('b')},
    c: function () {return this._compute_axis('c')},
    axes: function () {return this._compute_axes()}
  },


  methods: {
    _convert_length: function (value) {
      return this.state.imperial ? value / 25.4 : value;
    },


    _length_str: function (value) {
      return this._convert_length(value).toLocaleString() +
        (this.state.imperial ? ' in' : ' mm');
    },


    _compute_axis: function (axis) {
      var abs        = this.state[axis + 'p'] || 0;
      var off        = this.state['offset_' + axis];
      var motor_id   = this._get_motor_id(axis);
      var motor      = motor_id == -1 ? {} : this.config.motors[motor_id];
      var pm         = motor['power-mode'];
      var enabled    = typeof pm != 'undefined' && pm != 'disabled';
      var homingMode = motor['homing-mode']
      var homed      = this.state[motor_id + 'homed'];
      var min        = this.state[motor_id + 'tn'];
      var max        = this.state[motor_id + 'tm'];
      var dim        = max - min;
      var pathMin    = this.state['path_min_' + axis];
      var pathMax    = this.state['path_max_' + axis];
      var pathDim    = pathMax - pathMin;
      var under      = pathMin - off < min;
      var over       = max < pathMax - off;
      var klass      = (homed ? 'homed' : 'unhomed') + ' axis-' + axis;
      var state      = 'UNHOMED';
      var icon       = 'question-circle';
      var fault      = this.state[motor_id + 'df'] & 0x1f;
      var title;

      if (fault) {
        state = 'FAULT';
        klass += ' error';
        icon = 'exclamation-circle';

      } else if (0 < dim && dim < pathDim) {
        state = 'NO FIT';
        klass += ' error';
        icon = 'ban';

      } else if (homed) {
        state = 'HOMED'
        icon = 'check-circle';

        if (over || under) {
          state = over ? 'OVER' : 'UNDER';
          klass += ' warn';
          icon = 'exclamation-circle';
        }
      }

      switch (state) {
      case 'UNHOMED':  title = 'Click the home button to home axis.'; break;
      case 'HOMED': title = 'Axis successfuly homed.'; break;

      case 'OVER':
        title = 'Tool path would move ' +
          this._length_str(pathMax - off - max) + ' beyond axis bounds.';
        break;

      case 'UNDER':
        title = 'Tool path would move ' +
          this._length_str(min - pathMin + off) + ' below axis bounds.';
        break;

      case 'NO FIT':
        title = 'Tool path dimensions exceed axis dimensions by ' +
          this._length_str(pathDim - dim) + '.';
        break;

      case 'FAULT':
        title = 'Motor driver fault.  A potentially damaging electrical ' +
          'condition was detected and the motor driver was shutdown.  ' +
          'Please power down the controller and check your motor cabling.  ' +
          'See the "Motor Faults" table on the "Indicators" for more ' +
          'information.';
        break;
      }

      return {
        pos: abs + off,
        abs: abs,
        off: off,
        min: min,
        max: max,
        dim: dim,
        pathMin: pathMin,
        pathMax: pathMax,
        pathDim: pathDim,
        motor: motor_id,
        enabled: enabled,
        homingMode: homingMode,
        homed: homed,
        klass: klass,
        state: state,
        icon: icon,
        title: title
      }
    },


    _get_motor_id: function (axis) {
      for (var i = 0; i < this.config.motors.length; i++) {
        var motor = this.config.motors[i];
        if (motor.axis.toLowerCase() == axis) return i;
      }

      return -1;
    },


    _compute_axes: function () {
      var homed = false;

      for (var name of 'xyzabc') {
        var axis = this[name];

        if (!axis.enabled) continue
        if (!axis.homed) {homed = false; break}
        homed = true;
      }

      var error = false;
      var warn = false;

      if (homed)
        for (name of 'xyzabc') {
          axis = this[name];

          if (!axis.enabled) continue;
          if (axis.klass.indexOf('error') != -1) error = true;
          if (axis.klass.indexOf('warn') != -1) warn = true;
        }

      var klass = homed ? 'homed' : 'unhomed';
      if (error) klass += ' error';
      else if (warn) klass += ' warn';

      return {
        homed: homed,
        klass: klass
      }
    }
  }
}
