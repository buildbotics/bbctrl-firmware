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
  replace: true,
  template: '{{text}}<span class="unit">{{metric ? unit : iunit}}</span>',
  props: ['value', 'precision', 'unit', 'iunit', 'scale'],


  computed: {
    metric() {return !this.$root.state.imperial},


    text() {
      let value = this.value
      if (typeof value == 'undefined') return ''

      if (!this.metric) value /= this.scale

      return (1 * value.toFixed(this.precision)).toLocaleString()
    }
  },


  ready() {
    if (typeof this.precision == 'undefined') this.precision = 0
    if (typeof this.unit == 'undefined') this.unit = 'mm'
    if (typeof this.iunit == 'undefined') this.iunit = 'in'
    if (typeof this.scale == 'undefined') this.scale = 25.4
  }
}
