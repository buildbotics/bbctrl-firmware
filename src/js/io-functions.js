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



module.exports = {
  template: "#io-functions-template",
  props: ['template', 'state'],


  components: {
    'io-functions-row': {
      template: "#io-functions-row-template",
      props: ['func'],
      replace: true
    }
  },


  computed: {
    columns: () => ['function', 'state', 'pins', '',
                    'function', 'state', 'pins'],


    rows() {return Math.ceil(this.functions.length / 2.0)},


    functions() {
      let l = []
      let io_map = this.template['io-map']
      let pins = io_map.pins
      let tmpl = io_map.template.function
      let funcs = tmpl.values
      let codes = tmpl.codes
      let title = ''

      for (let i = 1; i < funcs.length; i++) {
        let p = []

        for (let j = 0; j < pins.length; j++) {
          let code = String.fromCharCode(97 + j) + 'io'
          if (i == this.state[code]) p.push(pins[j].id)
        }

        let state = this.state[codes[i]]
        let type = funcs[i].split('-')[0]
        if (!p.length) {
          type = 'disabled'
          title = 'Function not mapped to any pins.'
        }

        l.push({
          name: funcs[i],
          state,
          pins: p,
          type,
          title
        })
      }

      return l
    }
  }
}
