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

var api = require('./api');


function _is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
}


function escapeHTML(s) {
  var entityMap = {'&': '&amp;', '<': '&lt;', '>': '&gt;'};
  return String(s).replace(/[&<>]/g, function (s) {return entityMap[s];});
}


module.exports = {
  template: '#control-view-template',
  props: ['config', 'template', 'state'],


  data: function () {
    return {
      mdi: '',
      files: [],
      axes: 'xyzabc',
      history: [],
      speed_override: 1,
      feed_override: 1,
      manual_home: {x: false, y: false, z: false, a: false, b: false, c: false},
      position_msg:
      {x: false, y: false, z: false, a: false, b: false, c: false},
      axis_position: 0,
      video_url: '',
      deleteGCode: false
    }
  },


  components: {
    'axis-control': require('./axis-control'),
    'gcode-viewer': require('./gcode-viewer')
  },


  watch: {
    'state.line': function () {
      if (this.mach_state != 'HOMING')
        this.$broadcast('gcode-line', this.state.line);
    },


    'state.selected': function () {this.load()}
  },


  computed: {
    mach_state: function () {
      var cycle = this.state.cycle;
      var state = this.state.xx;

      if (typeof cycle != 'undefined' && state != 'ESTOPPED' &&
          (cycle == 'jogging' || cycle == 'homing'))
        return cycle.toUpperCase();
      return state || ''
    },


    is_running: function () {
      return this.mach_state == 'RUNNING' || this.mach_state == 'HOMING';
    },


    is_stopping: function() {return this.mach_state == 'STOPPING'},
    is_holding: function() {return this.mach_state == 'HOLDING'},
    is_ready: function() {return this.mach_state == 'READY'},


    reason: function () {
      if (this.mach_state == 'ESTOPPED') return this.state.er;
      if (this.mach_state == 'HOLDING') return this.state.pr;
      return '';
    },


    highlight_reason: function () {return this.reason != ''}
  },


  events: {
    jog: function (axis, power) {
      var data = {};
      data[axis] = power;
      api.put('jog', data);
    },

    connected: function () {this.update()}
  },


  ready: function () {
    this.update();
    this.load();
  },


  methods: {
    send: function (msg) {this.$dispatch('send', msg)},


    get_axis_motor_id: function (axis) {
      axis = axis.toLowerCase();

      for (var i = 0; i < this.config.motors.length; i++) {
        var motor = this.config.motors[i];
        if (motor.axis.toLowerCase() == axis) return i;
      }

      return -1;
    },


    get_axis_motor: function (axis) {
      var motor = this.get_axis_motor_id(axis);
      if (motor != -1) return this.config.motors[motor];
    },


    get_axis_motor_param: function (axis, name) {
      var motor = this.get_axis_motor(axis);
      if (typeof motor == 'undefined') return;
      if (typeof motor[name] != 'undefined') return motor[name];

      for (var section in this.template.motors) {
        var sec = this.template.motors[section];
        if (typeof sec[name] != 'undefined') return sec[name]['default'];
      }
    },


    enabled: function (axis) {
      var pm = this.get_axis_motor_param(axis, 'power-mode')
      return typeof pm != 'undefined' && pm != 'disabled';
    },


    is_homed: function (axis) {
      if (typeof axis == 'undefined') {
        var enabled = false;
        var axes = 'xyzabc';

        for (var i in axes) {
          if (this.enabled(axes.charAt(i))) {
            if (!this.is_homed(axes.charAt(i))) return false;
            else enabled = true;
          }
        }

        return enabled;

      } else {
        var motor = this.get_axis_motor_id(axis);
        return motor != -1 && this.state[motor + 'homed'];
      }
    },


    update: function () {
      // Update file list
      api.get('file').done(function (files) {
        this.files = files;
        this.load();
      }.bind(this))
    },


    load: function () {
      var file = this.state.selected;
      if (typeof file != 'undefined') this.$broadcast('gcode-load', file);
      this.$broadcast('gcode-line', this.state.line);
    },


    submit_mdi: function () {
      this.send(this.mdi);
      if (!this.history.length || this.history[0] != this.mdi)
        this.history.unshift(this.mdi);
      this.mdi = '';
    },


    mdi_start_pause: function () {
      if (this.state.xx == 'RUNNING') this.pause();

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause();

      else this.submit_mdi();
    },


    load_history: function (index) {this.mdi = this.history[index];},


    open: function (e) {
      $('.gcode-file-input').click();
    },


    upload: function (e) {
      var files = e.target.files || e.dataTransfer.files;
      if (!files.length) return;

      var file = files[0];
      var fd = new FormData();

      fd.append('gcode', file);

      api.upload('file', fd)
        .done(function () {
          this.$broadcast('gcode-reload', file.name);
          this.update();
        }.bind(this));
    },


    deleteCurrent: function () {
      if (this.state.selected)
        api.delete('file/' + this.state.selected).done(this.update);
      this.deleteGCode = false;
    },


    deleteAll: function () {
      api.delete('file').done(this.update);
      this.deleteGCode = false;
    },


    home: function (axis) {
      if (typeof axis == 'undefined') api.put('home');

      else {
        if (this.get_axis_motor_param(axis, 'homing-mode') != 'manual')
          api.put('home/' + axis);
        else this.manual_home[axis] = true;
      }
    },


    set_home: function (axis, position) {
      this.manual_home[axis] = false;
      api.put('home/' + axis + '/set', {position: parseFloat(position)});
    },


    show_set_position: function (axis) {
      this.axis_position = 0;
      this.position_msg[axis] = true;
    },


    get_offset: function (axis) {return this.state['offset_' + axis] || 0},


    set_position: function (axis, position) {
      this.position_msg[axis] = false;
      api.put('position/' + axis, {'position': parseFloat(position)});
    },


    zero: function (axis) {
      if (typeof axis == 'undefined') {
        var axes = 'xyzabc';
        for (var i in axes)
          if (this.enabled(axes.charAt(i)))
            this.zero(axes.charAt(i));

      } else this.set_position(axis, 0);
    },


    start_pause: function () {
      if (this.state.xx == 'RUNNING') this.pause();

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause();

      else this.start();
    },


    start: function () {api.put('start')},
    pause: function () {api.put('pause')},
    unpause: function () {api.put('unpause')},
    optional_pause: function () {api.put('pause/optional')},
    stop: function () {api.put('stop')},
    step: function () {api.put('step')},


    override_feed: function () {api.put('override/feed/' + this.feed_override)},


    override_speed: function () {
      api.put('override/speed/' + this.speed_override)
    },


    current: function (axis, value) {
      var x = value / 32.0;
      if (this.state[axis + 'pl'] == x) return;

      var data = {};
      data[axis + 'pl'] = x;
      this.send(JSON.stringify(data));
    },


    load_video: function () {
      this.video_url = '//' + document.location.hostname + ':8000/stream/0?=' +
        Math.random();
    },


    reload_video: function () {
      if (typeof this.lastVideoReset != 'undefined' &&
          Date.now() - this.lastVideoReset < 15000) return;

      this.lastVideoReset = Date.now();
      api.put('video/reload');
      setTimeout(this.load_video, 4000);
    }
  }
}
