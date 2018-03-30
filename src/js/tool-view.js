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


module.exports = {
  template: '#tool-view-template',
  props: ['config', 'template'],


  data: function () {
    return {
      tool: {},
      pwmSpindle: {},
      modbusSpindle: {}
    }
  },


  events: {
    'input-changed': function() {
      this.$dispatch('config-changed');
      return false;
    }
  },


  ready: function () {this.update()},


  methods: {
    get_type: function () {
      return (this.tool['spindle-type'] || 'Disabled').toUpperCase();
    },


    update_tool: function (type) {
      if (this.config.hasOwnProperty(type + '-spindle'))
        this[type + 'Spindle'] = this.config[type + '-spindle'];

      var template = this.template[type + '-spindle'];
      for (var key in template)
        if (!this[type + 'Spindle'].hasOwnProperty(key))
          this.$set(type + 'Spindle["' + key + '"]', template[key].default);
    },


    update: function () {
      Vue.nextTick(function () {
        if (this.config.hasOwnProperty('tool'))
          this.tool = this.config.tool;

        var template = this.template.tool;
        for (var key in template)
          if (!this.tool.hasOwnProperty(key))
            this.$set('tool["' + key + '"]', template[key].default);

        this.update_tool('pwm');
        this.update_tool('modbus');
       }.bind(this));
    }
  }
}
