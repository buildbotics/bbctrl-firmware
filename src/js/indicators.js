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
      return typeof this.state[motor + 'pm'] != 'undefined' &&
        this.state[motor + 'pm'];
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


    get_class: function (active, state) {
      if (typeof active == 'undefined' || typeof state == 'undefined')
        return 'fa-exclamation-triangle warn';

      if (state == 2) return 'fa-circle-o';

      return (state ? 'fa-plus-circle' : 'fa-minus-circle') + ' ' +
        (active ? 'active' : 'inactive');
    },


    get_input_active: function (stateCode, typeCode) {
      var type = this.state[typeCode];
      var state = this.state[stateCode];

      if (type == 1) return !state;     // Normally open
      else if (type == 2) return state; // Normally closed

      return false
    },


    get_input_class: function (stateCode, typeCode) {
      return this.get_class(this.get_input_active(stateCode, typeCode),
                            this.state[stateCode]);
    },


    get_output_class: function (output) {
      return this.get_class(this.state[output + 'oa'],
                            this.state[output + 'os']);
    },


    get_tooltip: function (mode, active, state) {
      if (typeof mode == 'undefined' || typeof active == 'undefined' ||
          typeof state == 'undefined') return 'Invalid';

      if (state == 0) state = 'Lo/Gnd';
      else if (state == 1) state = 'Hi/+3.3v';
      else if (state == 2) state = 'Tristated';
      else return 'Invalid';

      return 'Mode: ' + mode + '\nActive: ' + (active ? 'True' : 'False') +
        '\nLevel: ' + state;
    },


    get_input_tooltip: function (stateCode, typeCode) {
      var type = this.state[typeCode];
      if (type == 0) return 'Disabled';
      else if (type == 1) type = 'Normally open';
      else if (type == 2) type = 'Normally closed';

      var active = this.get_input_active(stateCode, typeCode);
      var state = this.state[stateCode];

      return this.get_tooltip(type, active, state);
    },


    get_output_tooltip: function (output) {
      var mode = this.state[output + 'om'];
      if (mode == 0) return 'Disabled';
      else if (mode == 1) mode = 'Lo/Hi';
      else if (mode == 2) mode = 'Hi/Lo';
      else if (mode == 3) mode = 'Tri/Lo';
      else if (mode == 4) mode = 'Tri/Hi';
      else if (mode == 5) mode = 'Lo/Tri';
      else if (mode == 6) mode = 'Hi/Tri';
      else mode = undefined;

      var active = this.state[output + 'oa'];
      var state = this.state[output + 'os'];

      return this.get_tooltip(mode, active, state);
    }
  }
}
