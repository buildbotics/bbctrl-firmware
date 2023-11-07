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



module.exports = {
  template: '#settings-motor-template',
  props: ['index', 'config', 'template', 'state'],


  computed: {
    metric() {return this.$root.metric()},


    is_slave() {
      for (let i = 0; i < this.index; i++)
        if (this.motor.axis == this.config.motors[i].axis)
          return true

      return false
    },


    motor() {return this.config.motors[this.index]},


    invalidMaxVelocity() {
      return this.maxMaxVelocity < this.motor['max-velocity']
    },


    maxStepsPerSecond() {return (0.5 / this.motor['step-length'])},


    maxMaxVelocity() {
      let maxUStepsPerSecond = 0.5 / this.motor['step-length']
      let uStepsPerRevolution =
          (360.0 / this.motor['step-angle']) * this.motor.microsteps
      let maxRPS = maxUStepsPerSecond / uStepsPerRevolution

      return parseFloat((maxRPS * 60.0 * this.motor['travel-per-rev'] / 1000)
                        .toFixed(3)) // returns max vel in meters / minute
    },


    ustepPerSec() {
      return this.rpm * this.stepsPerRev * this.motor.microsteps / 60
    },


    rpm() {
      return 1000 * this.motor['max-velocity'] / this.motor['travel-per-rev']
    },


    gForce() {return this.motor['max-accel'] * 0.0283254504},
    gForcePerMin() {return this.motor['max-jerk'] * 0.0283254504},
    stepsPerRev() {return 360 / this.motor['step-angle']},


    umPerStep() {
      return this.motor['travel-per-rev'] * this.motor['step-angle'] / 0.36
    },


    milPerStep() {return this.umPerStep / 25.4},


    invalidStallVelocity() {
      if (!this.motor['homing-mode'].startsWith('stall-')) return false
      return this.maxStallVelocity < this.motor['search-velocity']
    },


    stallRPM() {
      let v = this.motor['search-velocity']
      return 1000 * v / this.motor['travel-per-rev']
    },


    maxStallVelocity() {
      let maxRate  = 900000 / this.motor['stall-sample-time']
      let ustep    = this.motor['stall-microstep']
      let angle    = this.motor['step-angle']
      let travel   = this.motor['travel-per-rev']
      let maxStall = maxRate * 60 / 360 / 1000 * angle / ustep * travel

      return 1 * maxStall.toFixed(3)
    },


    stallUStepPerSec() {
      let ustep = this.motor['stall-microstep']
      return this.stallRPM * this.stepsPerRev * ustep / 60
    }
  },


  events: {
    'input-changed'() {
      Vue.nextTick(() => {
        // set step length to default of 2ms if user selects internal drivers
        if (this.motor['type-of-driver'] == 'internal')
          this.$set('motor["step-length"]', 0.000002)

        // Limit max-velocity
        if (this.invalidMaxVelocity)
          this.$set('motor["max-velocity"]', this.maxMaxVelocity)

        // Limit stall-velocity
        if (this.invalidStallVelocity)
          this.$set('motor["search-velocity"]', this.maxStallVelocity)


        this.$dispatch('config-changed')
      })
      return true
    }
  },


  methods: {
    show(name, templ) {
      if (name == 'step-length' && this.motor['type-of-driver'] == 'internal')
        return false

      if (this.motor['type-of-driver'] == 'generic external' &&
          name == 'homing-mode') return false

      if (this.motor['type-of-driver'] == 'internal' &&
          name == 'homing-mode-external') return false

      if ((this.motor['type-of-driver'] == 'generic external') &&
          ['stall-microstep', 'stall-volts', 'stall-sample-time',
           'stall-current'].includes(name)) return false

      if (templ.hmodes == undefined) return true

      if (this.motor['type-of-driver'] == 'internal')
        return templ.hmodes.indexOf(this.motor['homing-mode']) != -1

      return templ.hmodes.indexOf(this.motor['homing-mode-external']) != -1
    },


    showCategory(name) {
      if (name == 'power' && this.motor['type-of-driver'] == 'generic external')
        return false

      return true
    }
  }

}
