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
  template: "#io-indicator-template",
  props: {
    state: {type: Number},
    mode: String,
    type: String
  },


  computed: {
    klass: function () {
      let klass = ''

      if (this.type == 'disabled') return 'fa-ban'
      if (this.type == 'analog') return 'fa-circle-o'

      if (this.state == 0) klass = 'inactive'
      if (this.state == 1) klass = 'active'

      if (this.mode) {
        let parts = this.mode.split('-')
        let state = 'tri'

        if (this.state == 0) state = parts[0]
        if (this.state == 1) state = parts[1]

        if (state == 'lo')  return klass + ' fa-minus-circle'
        if (state == 'hi')  return klass + ' fa-plus-circle'
        if (state == 'tri') return klass + ' fa-circle-o'

      } else {
        if (this.state == 0)    return klass + ' fa-circle'
        if (this.state == 1)    return klass + ' fa-circle'
        if (this.state == 0xff) return klass + ' fa-ban'
      }

      return 'fa-exclamation-triangle warn'
    },


    tooltip: function () {
      let parts = []

      if (this.type) parts.push('Type: ' + this.type)
      if (this.mode) parts.push('Mode: ' + this.mode)
      if (this.type != 'analog')
        parts.push('Active: ' + (this.state == 1 ? 'True' : 'False'))

      return parts.join('\n')
    }
  }
}
