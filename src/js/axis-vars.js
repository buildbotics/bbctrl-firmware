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



function is_defined(x) {return typeof x != 'undefined'}


module.exports = {
  props: ['state', 'config'],


  computed: {
    x() {return this._compute_axis('x')},
    y() {return this._compute_axis('y')},
    z() {return this._compute_axis('z')},
    a() {return this._compute_axis('a')},
    b() {return this._compute_axis('b')},
    c() {return this._compute_axis('c')},
    axes() {return this._compute_axes()}
  },


  methods: {
    get_bounds() {throw 'Not implemented'},


    _convert_length(value) {
      return this.state.imperial ? value / 25.4 : value
    },


    _length_str(value) {
      return this._convert_length(value).toLocaleString() +
        (this.state.imperial ? ' in' : ' mm')
    },


    _compute_axis(axis) {
      let abs        = this.state[axis + 'p'] || 0
      let off        = this.state['offset_' + axis]
      let motor_id   = this._get_motor_id(axis)
      let motor      = motor_id == -1 ? {} : this.config.motors[motor_id]
      let enabled    = typeof motor.enabled != 'undefined' && motor.enabled
      let homingMode = motor['homing-mode']
      let homed      = this.state[motor_id + 'homed']
      let min        = this.state[motor_id + 'tn']
      let max        = this.state[motor_id + 'tm']
      let dim        = max - min
      let bounds     = this.toolpath.bounds
      let pathMin    = bounds ? bounds.min[axis] : 0
      let pathMax    = bounds ? bounds.max[axis] : 0
      let pathDim    = pathMax - pathMin
      let klass      = (homed ? 'homed' : 'unhomed') + ' axis-' + axis
      let state      = 'UNHOMED'
      let icon       = 'question-circle'
      let fault      = this.state[motor_id + 'df'] & 0x1f
      let shutdown   = this.state.power_shutdown
      let title

      if (fault || shutdown) {
        state = shutdown ? 'SHUTDOWN' : 'FAULT'
        klass += ' error'
        icon = 'exclamation-circle'

      } else if (0 < dim && dim < pathDim) {
        state = 'NO FIT'
        klass += ' error'
        icon = 'ban'

      } else if (homed) {
        state = 'HOMED'
        icon = 'check-circle'
      }

      switch (state) {
      case 'UNHOMED': title = 'Click the home button to home axis.'; break
      case 'HOMED': title = 'Axis successfuly homed.'; break

      case 'NO FIT':
        title = 'Tool path dimensions exceed axis dimensions by ' +
          this._length_str(pathDim - dim) + '.'
        break

      case 'FAULT':
        title = 'Motor driver fault.  A potentially damaging electrical ' +
          'condition was detected and the motor driver was shutdown.  ' +
          'Please power down the controller and check your motor cabling.  ' +
          'See the "Motor Faults" table on the "Indicators" tab for more ' +
          'information.'
        break

      case 'SHUTDOWN':
        title = 'Motor power fault.  All motors in shutdown.  ' +
          'See the "Power Faults" table on the "Indicators" tab for more ' +
          'information.  Reboot controller to reset.'
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


    _get_motor_id(axis) {
      for (let i = 0; i < this.config.motors.length; i++) {
        let motor = this.config.motors[i]
        if (motor.axis.toLowerCase() == axis) return i
      }

      return -1
    },


    _compute_axes() {
      let homed = false

      for (let name of 'xyzabc') {
        let axis = this[name]

        if (!axis.enabled) continue
        if (!axis.homed) {homed = false; break}
        homed = true
      }

      let error = false
      let warn = false

      if (homed)
        for (let name of 'xyzabc') {
          let axis = this[name]

          if (!axis.enabled) continue
          if (axis.klass.indexOf('error') != -1) error = true
          if (axis.klass.indexOf('warn') != -1) warn = true
        }

      let klass = homed ? 'homed' : 'unhomed'
      if (error) klass += ' error'
      else if (warn) klass += ' warn'

      return {
        homed: homed,
        klass: klass
      }
    }
  }
}
