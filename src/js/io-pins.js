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
  template: "#io-pins-template",
  props: ['template', 'state'],


  components: {
    'io-pins-row': {
      template: "#io-pins-row-template",
      props: ['pin'],
      replace: true
    }
  },


  computed: {
    columns: () => ['pin', 'state', 'function', '', 'pin', 'state', 'function'],
    rows() {return Math.ceil(this.io_pins.length / 2.0)},


    io_pins() {
      let l         = []
      let io_map    = this.template['io-map']
      let pins      = io_map.pins
      let modes     = io_map.template.mode.values
      let functions = io_map.template.function.values

      for (let i = 0; i < pins.length; i++) {
        let c     = String.fromCharCode(97 + i)
        let func  = functions[this.state[c + 'io']]
        let state = this.state[c + 'is']
        let type  = pins[i].type
        let mode  = modes[this.state[c + 'im']]

        l.push({id: pins[i].id, state, mode, func, type})
      }

      let fixed = [
        {id:  6, func: '0-10v',    type: 'analog' },
        {id:  7, func: 'ground',   type: 'ground' },
        {id: 13, func: 'rs485-a',  type: 'digital'},
        {id: 14, func: 'rs485-b',  type: 'digital'},
        {id: 17, func: 'tool-pwm', type: 'digital'},
        {id: 19, func: 'ground',   type: 'ground' },
        {id: 20, func: '5v',       type: 'power'  },
        {id: 25, func: 'ground',   type: 'ground' }
      ]

      return l.concat(fixed).sort((a, b) => b.id < a.id)
    }
  }
}
