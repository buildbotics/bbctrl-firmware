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


module.exports = {
  template: '#settings-motor-template',
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
      return 1 * (15 * this.umPerStep / this.motor['microsteps']).toFixed(3);
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


    milPerStep: function () {return this.umPerStep / 25.4},


    invalidStallVelocity: function () {
      if (!this.motor['homing-mode'].startsWith('stall-')) return false;
      return this.maxStallVelocity < this.motor['search-velocity'];
    },


    stallRPM: function () {
      var v = this.motor['search-velocity'];
      return 1000 * v / this.motor['travel-per-rev'];
    },


    maxStallVelocity: function () {
      var maxRate  = 900000 / this.motor['stall-sample-time'];
      var ustep    = this.motor['stall-microstep'];
      var angle    = this.motor['step-angle'];
      var travel   = this.motor['travel-per-rev'];
      var maxStall = maxRate * 60 / 360 / 1000 * angle / ustep * travel;

      return 1 * maxStall.toFixed(3);
    },


    stallUStepPerSec: function () {
      var ustep = this.motor['stall-microstep'];
      return this.stallRPM * this.stepsPerRev * ustep / 60;
    }
  },


  events: {
    'input-changed': function() {
      Vue.nextTick(function () {
        // Limit max-velocity
        if (this.invalidMaxVelocity)
          this.$set('motor["max-velocity"]', this.maxMaxVelocity);

        // Limit stall-velocity
        if (this.invalidStallVelocity)
          this.$set('motor["search-velocity"]', this.maxStallVelocity);

        this.$dispatch('config-changed');
      }.bind(this))
      return true;
    }
  },


  methods: {
    show: function (name, templ) {
      if (templ.hmodes == undefined) return true;
      return templ.hmodes.indexOf(this.motor['homing-mode']) != -1;
    }
  }
}
