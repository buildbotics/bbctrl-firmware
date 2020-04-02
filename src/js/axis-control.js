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
  template: '#axis-control-template',
  props: ['axes', 'colors', 'enabled', 'adjust', 'step'],


  methods: {
    jog: function (axis, ring, direction) {
      var value = direction * this.value(ring);
      this.$dispatch(this.step ? 'step' : 'jog', this.axes[axis], value);
    },


    release: function (axis) {
      if (!this.step) this.$dispatch('jog', this.axes[axis], 0)
    },


    value: function (ring) {
      var adjust = [0.01, 0.1, 1][this.adjust];
      if (this.step) return adjust * [0.1, 1, 10, 100][ring];
      return adjust * [0.1, 0.25, 0.5, 1][ring];
    },


    text: function (ring) {
      var value = this.value(ring) * (this.step ? 1 : 100);
      value = parseFloat(value.toFixed(3));
      return value + (this.step ? '' : '%');
    }
  }
}
