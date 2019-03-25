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

'use strict'


module.exports = {
  replace: true,
  template: '#templated-input-template',
  props: ['name', 'model', 'template'],


  data: function () {return {view: ''}},


  computed: {
    metric: function () {return this.$root.metric()},


    _view: function () {
      if (this.template.scale) {
        if (this.metric) return 1 * this.model.toFixed(3);

        return 1 * (this.model / this.template.scale).toFixed(4);
      }

      return this.model;
    },


    units: function () {
      return (this.metric || !this.template.iunit) ?
        this.template.unit : this.template.iunit;
    },


    title: function () {
      var s = 'Default ' + this.template.default + ' ' +
          (this.template.unit || '');
      if (typeof this.template.help != 'undefined')
        s = this.template.help + '\n' + s;
      return s;
    }
  },


  watch: {
    _view: function () {this.view = this._view},


    view: function () {
      if (this.template.scale && !this.metric)
        this.model = this.view * this.template.scale;
      else this.model = this.view;
    }
  },


  ready: function () {this.view = this._view},


  methods: {
    change: function () {this.$dispatch('input-changed')}
  }
}
