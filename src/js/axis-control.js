/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2026, Buildbotics LLC, All rights reserved.

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
  template: '#axis-control-template',
  props: ['axes', 'colors', 'enabled', 'adjust', 'step', 'disabled'],


  methods: {
    jog(axis, ring, direction) {
      let value = direction * this.value(ring)
      this.$dispatch(this.step ? 'step' : 'jog', this.axes[axis], value)
    },


    release(axis) {
      if (!this.step) this.$dispatch('jog', this.axes[axis], 0)
    },


    value(ring) {
      let adjust = [0.01, 0.1, 1][this.adjust]
      if (this.step) return adjust * [0.1, 1, 10, 100][ring]
      return adjust * [0.1, 0.25, 0.5, 1][ring]
    },


    text(ring) {
      let value = this.value(ring) * (this.step ? 1 : 100)
      value = parseFloat(value.toFixed(3))
      return value + (this.step ? '' : '%')
    }
  }
}
