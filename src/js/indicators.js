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


module.exports = {
  template: '#indicators-template',
  props: ['state'],


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


    get_class: function (code) {
      if (typeof this.state[code] != 'undefined') {
        var state = this.state[code];

        if (state == 0 || state === false) return 'logic-lo fa-circle';
        if (state == 1 || state === true) return 'logic-hi fa-circle';
        if (state == 2) return 'logic-tri fa-circle-o';
      }

      return 'fa-exclamation-triangle warn';
    }
  }
}
