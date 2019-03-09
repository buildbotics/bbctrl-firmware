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

var modbus = require('./modbus.js');


module.exports = {
  template: '#indicators-template',
  props: ['state'],


  computed: {
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


    motor_fault_class: function (motor, fault) {
      if (typeof motor == 'undefined') {
        var status = this.state['fa'];
        if (typeof status == 'undefined') return 'fa-question';
        return 'fa-thumbs-' + (status ? 'down error' : 'up success')
      }

      var status = this.state[motor + 'ds'];
      if (typeof status == 'undefined') return 'fa-question';
      return status.indexOf(fault) == -1 ? 'fa-thumbs-up success' :
        'fa-thumbs-down error';
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
