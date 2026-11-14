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



let cookie = require('./cookie')


// Step sizes in the machine's active units, smallest first.  Decimals are
// the places shown on the buttons, enough for the smallest step.
const increments = {
  metric:   {steps: [0.001, 0.01, 0.1, 1, 10],     decimals: 3},
  imperial: {steps: [0.0001, 0.001, 0.01, 0.1, 1], decimals: 4}
}


// The jog feed slider moves in 10% steps.  Snap a value saved before then,
// or a bad cookie, onto one.
function saved_feed() {
  let feed = Math.round(parseInt(cookie.get('jog-feed', 30)) / 10) * 10
  if (isNaN(feed)) return 30
  return Math.min(100, Math.max(10, feed))
}


// One axis at a time: hold +/- to jog continuously at a fraction of the
// axis' max velocity, or press once to step by a fixed increment.
//
// Nothing on the controller times out a continuous jog; it moves until it
// is sent zero power.  So every way a press can end must call release().
module.exports = {
  template: '#jog-control-template',
  props: ['enabled', 'metric', 'disabled'],


  data() {
    return {
      axis:     cookie.get('jog-axis', 'X'),
      step:     cookie.get_bool('jog-step'),
      feed:     saved_feed(),
      inc_index: parseInt(cookie.get('jog-increment', 2)),
      held:     0 // Direction of the continuous jog under way, or 0
    }
  },


  computed: {
    axes() {return 'XYZA'.split('')},
    unit_increments() {return increments[this.metric ? 'metric' : 'imperial']},
    increments() {return this.unit_increments.steps},
    increment() {return this.increments[this.inc_index]},
    units() {return this.metric ? 'mm' : 'in'},
    axis_enabled() {return !!this.enabled[this.axis.toLowerCase()]}
  },


  watch: {
    axis() {cookie.set('jog-axis', this.axis)},
    step() {cookie.set_bool('jog-step', this.step)},
    inc_index() {cookie.set('jog-increment', this.inc_index)},


    feed() {
      cookie.set('jog-feed', this.feed)
      if (this.held) this.send_jog(this.held) // Follow the slider
    },


    disabled(disabled) {if (disabled) this.release()}
  },


  ready() {
    window.addEventListener('blur', this.release)
    document.addEventListener('visibilitychange', this.release)
  },


  detached() {this.release()},


  beforeDestroy() {
    this.release()
    window.removeEventListener('blur', this.release)
    document.removeEventListener('visibilitychange', this.release)
  },


  methods: {
    format_increment(inc) {
      return inc.toFixed(this.unit_increments.decimals)
    },


    select_axis(axis) {
      if (this.held) return
      this.axis = axis
    },


    // Pointer events cover mouse and touch
    press(direction, e) {
      if (this.disabled || !this.axis_enabled) return
      if (e.button) return // Left button or touch only

      if (this.step) {
        let value = direction * this.increment
        return this.$dispatch('step', this.axis, value)
      }

      e.currentTarget.setPointerCapture(e.pointerId)
      this.send_jog(direction)
    },


    // A long touch would open the context menu and cancel the hold
    ignore(e) {e.preventDefault()},


    release() {
      if (!this.held) return
      this.held = 0
      this.$dispatch('jog', this.axis, 0)
    },


    send_jog(direction) {
      this.held = direction
      this.$dispatch('jog', this.axis, direction * this.feed / 100)
    }
  }
}
