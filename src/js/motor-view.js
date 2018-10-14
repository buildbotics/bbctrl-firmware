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
  template: '#motor-view-template',
  props: ['index', 'config', 'template', 'state'],


  computed: {
    metric: function () {return this.$root.metric()},


    is_slave: function () {
      for (var i = 0; i < this.index; i++)
        if (this.motor.axis == this.config.motors[i].axis)
          return true;

      return false;
    },


    motor: function () {return this.config.motors[this.index]},


    invalidMaxVelocity: function () {
      return this.maxMaxVelocity < this.motor['max-velocity'];
    },


    maxMaxVelocity: function () {
      return 15 * this.umPerStep / this.motor['microsteps'];
    },


    ustepPerSec: function () {
      return this.rpm * this.stepsPerRev * this.motor['microsteps'] / 60;
    },


    rpm: function () {
      return 1000 * this.motor['max-velocity'] / this.motor['travel-per-rev'];
    },


    gForce: function () {return this.motor['max-accel'] * 0.0283254504},
    gForcePerMin: function () {return this.motor['max-jerk'] * 0.0283254504},
    stepsPerRev: function () {return 360 / this.motor['step-angle']},


    umPerStep: function () {
      return this.motor['travel-per-rev'] * this.motor['step-angle'] / 0.36
    },


    milPerStep: function () {return this.umPerStep / 25.4}
  },


  events: {
    'input-changed': function() {
      // Limit max-velocity
      if (this.invalidMaxVelocity)
        this.motor['max-velocity'] = this.maxMaxVelocity;

      this.$dispatch('config-changed');
      return false;
    }
  }
}
