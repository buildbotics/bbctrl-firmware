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


let modbus = require('./modbus.js')


module.exports = {
  template: '#settings-tool-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      address: 0,
      value: 0
    }
  },


  components: {'modbus-reg': require('./modbus-reg.js')},


  watch: {
    'state.mr'() {this.value = this.state.mr}
  },


  ready() {this.value = this.state.mr},


  computed: {
    regs_tmpl() {return this.template['modbus-spindle'].regs},
    tool_type() {return this.config.tool['tool-type'].toUpperCase()},


    is_modbus() {
      return this.tool_type != 'DISABLED' && this.tool_type != 'PWM SPINDLE'
    },


    modbus_status() {return modbus.status_to_string(this.state.mx)}
  },


  methods: {
    get_reg_type(reg) {
      return this.regs_tmpl.template['reg-type'].values[this.state[reg + 'vt']]
    },


    get_reg_addr(reg) {return this.state[reg + 'va']},
    get_reg_value(reg) {return this.state[reg + 'vv']},


    get_reg_fails(reg) {
      let fails = this.state[reg + 'vr']
      return fails == 255 ? 'Max' : fails
    },


    show_modbus_field(key) {
      return key != 'regs' &&
        (key != 'multi-write' || this.tool_type == 'CUSTOM MODBUS VFD')
    },


    customize(e) {
      this.config.tool['tool-type'] = 'Custom Modbus VFD'

      let regs = this.config['modbus-spindle'].regs
      for (let i = 0; i < regs.length; i++) {
        let reg = this.regs_tmpl.index[i]
        regs[i]['reg-type']  = this.get_reg_type(reg)
        regs[i]['reg-addr']  = this.get_reg_addr(reg)
        regs[i]['reg-value'] = this.get_reg_value(reg)
      }

      this.$dispatch('config-changed')
    },


    clear(e) {
      this.config.tool['tool-type'] = 'Custom Modbus VFD'

      let regs = this.config['modbus-spindle'].regs
      for (let i = 0; i < regs.length; i++) {
        regs[i]['reg-type']  = 'disabled'
        regs[i]['reg-addr']  = 0
        regs[i]['reg-value'] = 0
      }

      this.$dispatch('config-changed')
    },


    reset_failures(e) {
      let regs = this.config['modbus-spindle'].regs
      for (let reg = 0; reg < regs.length; reg++)
        this.$dispatch('send', '\$' + reg + 'vr=0')
    }
  }
}
