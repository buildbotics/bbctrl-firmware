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



// Exactly `precision` decimals, with locale digit grouping.  Rounds first so
// a tiny negative value shows as 0.000 rather than -0.000.
function fixed_str(value, precision) {
  let scale = Math.pow(10, precision)
  let rounded = Math.round(value * scale) / scale
  if (rounded == 0) rounded = 0

  return rounded.toLocaleString(undefined, {
    minimumFractionDigits: precision, maximumFractionDigits: precision})
}


module.exports = {
  replace: true,
  template: '{{text}}<span class="unit">{{metric ? unit : iunit}}</span>',
  // `iprecision` overrides `precision` in imperial units.  With `fixed` the
  // value always shows that many decimals, trailing zeros included.
  props: ['value', 'precision', 'iprecision', 'fixed', 'unit', 'iunit',
          'scale'],


  computed: {
    metric() {return !this.$root.state.imperial},


    text() {
      let value = this.value
      if (typeof value == 'undefined') return ''

      let precision = this.precision
      if (!this.metric) {
        value /= this.scale
        if (typeof this.iprecision != 'undefined') precision = this.iprecision
      }

      if (this.fixed) return fixed_str(value, precision)
      return (1 * value.toFixed(precision)).toLocaleString()
    }
  },


  ready() {
    if (typeof this.precision == 'undefined') this.precision = 0
    if (typeof this.unit == 'undefined') this.unit = 'mm'
    if (typeof this.iunit == 'undefined') this.iunit = 'in'
    if (typeof this.scale == 'undefined') this.scale = 25.4
  }
}
