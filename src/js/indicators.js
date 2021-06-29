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

var modbus = require('./modbus.js');


module.exports = {
  template: '#indicators-template',
  props: ['state'],


  computed: {
    pwr_version: function () {
      var v = parseInt(this.state.pwr_version)
      return isNaN(v) ? 0 : ((v >> 8) + '.' + (v & 0xff))
    },


    watts: function () {
      var I = parseFloat(this.state.motor)
      var V = parseFloat(this.state.vout)
      return I * V
    },


    modbus_status: function () {return modbus.status_to_string(this.state.mx)},


    sense_error: function () {
      var error = '';

      if (this.state.motor_voltage_sense_error) error += 'Motor voltage\n';
      if (this.state.motor_current_sense_error) error += 'Motor current\n';
      if (this.state.load1_sense_error) error += 'Load 1\n';
      if (this.state.load2_sense_error) error += 'Load 2\n';
      if (this.state.vdd_current_sense_error) error += 'Vdd current\n';

      return error;
    }
  },


  methods: {
    is_motor_enabled: function (motor) {
      return typeof this.state[motor + 'me'] != 'undefined' &&
        this.state[motor + 'me'];
    },


    get_min_pin: function (motor) {
      switch (motor) {
      case 0: return 3;
      case 1: return 5;
      case 2: return 9;
      case 3: return 11;
      }
    },


    get_max_pin: function (motor) {
      switch (motor) {
      case 0: return 4;
      case 1: return 8;
      case 2: return 10;
      case 3: return 12;
      }
    },


    motor_fault_class: function (motor, bit) {
      if (typeof motor == 'undefined') {
        var status = this.state['fa'];
        if (typeof status == 'undefined') return 'fa-question';
        return 'fa-thumbs-' + (status ? 'down error' : 'up success')
      }

      var flags = this.state[motor + 'df'];
      if (typeof flags == 'undefined') return 'fa-question';
      return (flags & (1 << bit)) ? 'fa-thumbs-down error' :
        'fa-thumbs-up success';
    },


    motor_reset: function (motor) {
      if (typeof motor == 'undefined') {
        var cmd = '';
        for (var i = 0; i < 4; i++)
          cmd += '\\$' + i + 'df=0\n';
        this.$dispatch('send', cmd);

      } else this.$dispatch('send', '\\$' + motor + 'df=0');
    }
  }
}
