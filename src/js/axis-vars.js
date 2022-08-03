/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

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
    get_bounds() {throw 'Not implemented'},


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
      var enabled    = typeof motor.enabled != 'undefined' && motor.enabled;
      var homingMode = motor['homing-mode']
      var homed      = this.state[motor_id + 'homed'];
      var min        = this.state[motor_id + 'tn'];
      var max        = this.state[motor_id + 'tm'];
      var dim        = max - min;
      let bounds     = this.toolpath.bounds
      var pathMin    = bounds ? bounds.min[axis] : 0;
      var pathMax    = bounds ? bounds.max[axis] : 0;
      var pathDim    = pathMax - pathMin;
      var klass      = (homed ? 'homed' : 'unhomed') + ' axis-' + axis;
      var state      = 'UNHOMED';
      var icon       = 'question-circle';
      var fault      = this.state[motor_id + 'df'] & 0x1f;
      var shutdown   = this.state.power_shutdown;
      var title;

      if (fault || shutdown) {
        state = shutdown ? 'SHUTDOWN' : 'FAULT';
        klass += ' error';
        icon = 'exclamation-circle';

      } else if (0 < dim && dim < pathDim) {
        state = 'NO FIT';
        klass += ' error';
        icon = 'ban';

      } else if (homed) {
        state = 'HOMED'
        icon = 'check-circle';
      }

      switch (state) {
      case 'UNHOMED': title = 'Click the home button to home axis.'; break;
      case 'HOMED': title = 'Axis successfuly homed.'; break;

      case 'NO FIT':
        title = 'Tool path dimensions exceed axis dimensions by ' +
          this._length_str(pathDim - dim) + '.';
        break;

      case 'FAULT':
        title = 'Motor driver fault.  A potentially damaging electrical ' +
          'condition was detected and the motor driver was shutdown.  ' +
          'Please power down the controller and check your motor cabling.  ' +
          'See the "Motor Faults" table on the "Indicators" tab for more ' +
          'information.';
        break;

      case 'SHUTDOWN':
        title = 'Motor power fault.  All motors in shutdown.  ' +
          'See the "Power Faults" table on the "Indicators" tab for more ' +
          'information.  Reboot controller to reset.';
      }

      return {
        pos: abs - off,
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
        let axis = this[name];

        if (!axis.enabled) continue
        if (!axis.homed) {homed = false; break}
        homed = true;
      }

      var error = false;
      var warn = false;

      if (homed)
        for (name of 'xyzabc') {
          let axis = this[name];

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
