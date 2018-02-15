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


module.exports = Vue.extend({
  template: '<div class="gauge"><canvas></canvas><span></span></div>',


  props: {
    value: {tupe: Number, required: true},
    lines: {type: Number, default: 1},
    angle: {type: Number, default: 0.1},
    lineWidth: {type: Number, default: 0.25},
    ptrLength: {type: Number, default: 0.85},
    ptrStrokeWidth: {type: Number, default: 0.054},
    ptrColor: {type: String, default: '#666'},
    limitMax: {type: Boolean, default: true},
    colorStart: {type: String, default: '#6FADCF'},
    colorStop: {type: String, default: '#8FC0DA'},
    strokeColor: String,
    min: {type: Number, default: 0},
    max: {type: Number, default: 100}
  },


  data: function () {
    return {
    }
  },


  watch: {
    value: function (value) {this.set(value)}
  },


  compiled: function () {
    this.gauge = new Gauge(this.$el.children[0]).setOptions({
      lines: this.lines,
      angle: this.angle,
      lineWidth: this.lineWidth,
      pointer: {
        length: this.ptrLength,
        strokeWidth: this.ptrStrokeWidth,
        color: this.ptrColor
      },
      limitMax: this.limitMax,
      colorStart: this.colorStart,
      colorStop: this.colorStop,
      strokeColor: this.strokeColor,
      minValue: this.min,
      maxValue: this.max
   });

    this.gauge.minValue = parseInt(this.min);
    this.gauge.maxValue = parseInt(this.max);
    this.set(this.value);

    this.gauge.setTextField(this.$el.children[1]);
  },


  methods: {
    set: function (value) {
      if (typeof value == 'number') this.gauge.set(value);
    }
  }
})
