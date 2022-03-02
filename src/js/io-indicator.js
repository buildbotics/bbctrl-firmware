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


function get_state_class(state) {
  switch (state) {
  case 0: case 1: return 'fa-circle'
  case      'lo': return 'fa-minus-circle'
  case      'hi': return 'fa-plus-circle'
  case     'tri': return 'fa-circle-o'
  default:        return 'fa-exclamation-triangle warn'
  }
}


module.exports = {
  template: "#io-indicator-template",
  props: {
    state: {type: Number},
    mode: String,
    type: String,
    func: String
  },


  computed: {
    klass: function () {
      if (this.func == 'disabled' || this.type == 'disabled') return 'fa-ban'
      if (this.type == 'analog') return 'fa-random'

      let klass = ''
      let state = this.state

      if (state == 0) klass = 'inactive'
      if (state == 1) klass = 'active'

      if (this.mode) {
        if (this.type == 'output') {
          let parts = this.mode.split('-')

          switch (state) {
          case 0: case 1: state = parts[state]; break
          default: return 'fa-exclamation-triangle warn'
          }

        } else if (this.type == 'input') {
          let no = this.mode == 'normally-open'

          switch (state) {
          case 0: state = no ? 'hi' : 'lo'; break
          case 1: state = no ? 'lo' : 'hi'; break
          default: return 'fa-exclamation-triangle warn'
          }
        }
      }

      return klass + ' ' + get_state_class(state)
    },


    tooltip: function () {
      let parts = []

      if (this.type) parts.push('Type: ' + this.type)
      if (this.mode) parts.push('Mode: ' + this.mode)
      if (this.type != 'analog' && this.type != 'disabled')
        parts.push('Active: ' + (this.state == 1 ? 'True' : 'False'))

      return parts.join('\n')
    }
  }
}
