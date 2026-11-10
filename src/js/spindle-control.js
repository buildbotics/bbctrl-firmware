/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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



// Manual spindle start and stop, run as MDI commands.  Fwd and Rev run at
// the typed speed, or at the last programmed S speed if the box is empty.
module.exports = {
  template: '#spindle-control-template',
  props: ['state', 'disabled'],


  data() {
    return {
      target: '', // Typed speed, as text so it may contain commas
      editing: false
    }
  },


  computed: {
    // The controller reports reverse as a negative speed
    speed() {return this.state.s || 0},
    rpm() {return Math.abs(this.speed)},
    forward() {return 0 < this.speed},
    reverse() {return this.speed < 0},


    // Typed speed in RPM, undefined if empty, NaN if not a speed
    target_rpm() {
      let text = String(this.target).replace(/[,\s]/g, '')
      if (!text.length) return undefined
      if (!/^\d+(\.\d*)?$/.test(text)) return NaN
      return parseFloat(text)
    },


    invalid() {return Number.isNaN(this.target_rpm)},


    rpm_title() {
      return 'Spindle speed for Fwd and Rev.  Measured: ' +
        Math.round(this.rpm) + ' RPM'
    }
  },


  watch: {
    // Follow the programmed speed until the user types their own
    'state.speed'(speed) {if (!this.editing) this.target = speed || ''}
  },


  ready() {this.target = this.state.speed || ''},


  methods: {
    start(gcode) {
      if (this.invalid) return
      if (this.target_rpm != undefined) gcode += ' S' + this.target_rpm
      this.send(gcode)
      this.editing = false
    },


    send(gcode) {this.$dispatch('send', gcode)}
  }
}
