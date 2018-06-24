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
  template: '{{text}}<span class="unit">{{metric ? unit : iunit}}</span>',
  props: ['value', 'precision', 'unit', 'iunit', 'scale'],


  computed: {
    metric: function () {return this.$root.metric()},


    text: function () {
      var value = this.value;
      if (typeof value == 'undefined') return '';

      if (!this.metric) value /= this.scale;

      return (1 * value.toFixed(this.precision)).toLocaleString();
    }
  },


  ready: function () {
    if (typeof this.precision == 'undefined') this.precision = 0;
    if (typeof this.unit == 'undefined') this.unit = 'mm';
    if (typeof this.iunit == 'undefined') this.iunit = 'in';
    if (typeof this.scale == 'undefined') this.scale = 25.4;
  }
}
