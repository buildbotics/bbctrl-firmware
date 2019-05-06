/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

'use strict';

var api = require('./api');
var modbus = require('./modbus.js');


module.exports = {
  template: '#tool-view-template',
  props: ['config', 'template', 'state'],


  data: function () {
    return {
      address: 0,
      value: 0
    }
  },


  components: {'modbus-reg': require('./modbus-reg.js')},


  watch: {
    'state.mr': function () {this.value = this.state.mr}
  },


  events: {
    'input-changed': function() {
      this.$dispatch('config-changed');
      return false;
    }
  },


  ready: function () {this.value = this.state.mr},


  computed: {
    regs_tmpl: function () {return this.template['modbus-spindle'].regs},
    tool_type: function () {return this.config.tool['tool-type'].toUpperCase()},


    is_modbus: function () {
      return this.tool_type != 'DISABLED' && this.tool_type != 'PWM SPINDLE'
    },


    modbus_status: function () {return modbus.status_to_string(this.state.mx)}
  },


  methods: {
    get_reg_type: function (reg) {
      return this.regs_tmpl.template['reg-type'].values[this.state[reg + 'vt']]
    },


    get_reg_addr: function (reg) {return this.state[reg + 'va']},
    get_reg_value: function (reg) {return this.state[reg + 'vv']},


    get_reg_fails: function (reg) {
      var fails = this.state[reg + 'vr']
      return fails == 255 ? 'Max' : fails;
    },


    show_modbus_field: function (key) {
      return key != 'regs' &&
        (key != 'multi-write' || this.tool_type == 'CUSTOM MODBUS VFD');
    },


    read: function (e) {
      e.preventDefault();
      api.put('modbus/read', {address: this.address});
    },


    write: function (e) {
      e.preventDefault();
      api.put('modbus/write', {address: this.address, value: this.value});
    },


    customize: function (e) {
      e.preventDefault();
      this.config.tool['tool-type'] = 'Custom Modbus VFD';

      var regs = this.config['modbus-spindle'].regs;
      for (var i = 0; i < regs.length; i++) {
        var reg = this.regs_tmpl.index[i];
        regs[i]['reg-type']  = this.get_reg_type(reg);
        regs[i]['reg-addr']  = this.get_reg_addr(reg);
        regs[i]['reg-value'] = this.get_reg_value(reg);
      }

      this.$dispatch('config-changed');
    },


    clear: function (e) {
      e.preventDefault();
      this.config.tool['tool-type'] = 'Custom Modbus VFD';

      var regs = this.config['modbus-spindle'].regs;
      for (var i = 0; i < regs.length; i++) {
        regs[i]['reg-type']  = 'disabled';
        regs[i]['reg-addr']  = 0;
        regs[i]['reg-value'] = 0;
      }

      this.$dispatch('config-changed');
    },


    reset_failures: function (e) {
      e.preventDefault();
      var regs = this.config['modbus-spindle'].regs;
      for (var reg = 0; reg < regs.length; reg++)
        this.$dispatch('send', '\$' + reg + 'vr=0');
    }
  }
}
