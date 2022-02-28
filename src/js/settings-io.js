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
  template: '#settings-io-template',
  props: ['config', 'template', 'state'],

  computed: {
    io() {
      let io        = []
      let config    = this.config['io-map']
      let template  = this.template['io-map']
      let indices   = template.index
      let functions = template.template.function.values
      let modes     = template.template.mode.values
      let omodes    = modes.slice(0, 5)
      let imodes    = modes.slice(6, 8)

      for (let i = 0; i < indices.length; i++) {
        let c = String.fromCharCode(97 + i)

        let type = template.pins[i].type
        let funcs = functions.filter(name => name.startsWith(type))
        funcs.unshift('disabled')

        let modes = []
        if (type == 'output') modes = omodes
        if (type == 'input')  modes = imodes

        io.push({
          id: template.pins[i].id,
          type,
          state: this.state[c + 'is'],
          config: config[i],
          functions: funcs,
          modes
        })
      }

      return io
    }
  }
}
