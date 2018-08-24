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


    units: function () {
      return (this.metric || !this.template.iunit) ?
        this.template.unit : this.template.iunit;
    }
  },


  watch: {
    metric: function () {this.set_view()},
    model: function () {this.set_view()}
  },


  ready: function () {this.set_view()},


  methods: {
    set_view: function () {
      if (this.template.scale && !this.metric)
        this.view = (this.model / this.template.scale).toFixed(3);
      else this.view = this.model;
    },


    change: function () {
      if (this.template.scale && !this.metric)
        this.model = 1 * (this.view * this.template.scale).toFixed(4);
      else this.model = this.view;

      this.$dispatch('input-changed');
    }
  }
}
