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

var maxLines = 1000;
var pageSize = Math.round(maxLines / 10);


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
      last_file: '',
      files: [],
      axes: 'xyzabc',
      all_gcode: [],
      gcode: [],
      gcode_offset: 0,
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
    'axis-control': require('./axis-control')
  },


  watch: {
    'state.line': function () {this.update_gcode_line()},
    'state.selected': function () {this.load()}
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
    get_state: function () {
      if (typeof this.state.cycle != 'undefined' &&
          this.state.cycle != 'idle' && this.state.xx == 'RUNNING')
        return this.state.cycle.toUpperCase();
      return this.state.xx || ''
    },


    get_reason: function () {
      if (this.state.xx == 'ESTOPPED') return this.state.er;
      if (this.state.xx == 'HOLDING') return this.state.pr;
      return '';
    },


    highlight_reason: function () {return this.get_reason() != ''},
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
      var motor = this.get_axis_motor_id(axis);
      return motor != -1 && this.state[motor + 'homed'];
    },


    gcode_move_up: function (count) {
      var lines = 0;

      for (var i = 0; i < count; i++) {
        if (!this.gcode_offset) break;

        this.gcode.unshift(this.all_gcode[this.gcode_offset - 1])
        this.gcode.pop();
        this.gcode_offset--;
        lines++;
      }

      return lines;
    },


    gcode_move_down: function (count) {
      var lines = 0;

      for (var i = 0; i < count; i++) {
        if (this.all_gcode.length <= this.gcode_offset + this.gcode.length)
          break;

        this.gcode.push(this.all_gcode[this.gcode_offset + this.gcode.length])
        this.gcode.shift();
        this.gcode_offset++;
        lines++
      }

      return lines;
    },


    gcode_scroll: function (e) {
      if (this.gcode.length == this.all_gcode.length) return;

      var t = e.target;
      var percentScroll = t.scrollTop / (t.scrollHeight - t.clientHeight);

      var lines = 0;
      if (percentScroll < 0.2) lines = this.gcode_move_up(pageSize);
      else if (0.8 < percentScroll) lines = -this.gcode_move_down(pageSize);
      else return;

      if (lines) t.scrollTop += t.scrollHeight * lines / maxLines;
    },


    update_gcode_line: function () {
      if (typeof this.last_line != 'undefined') {
        $('#gcode-line-' + this.last_line).removeClass('highlight');
        this.last_line = undefined;
      }

      if (0 <= this.state.line) {
        var line = this.state.line - 1;

        // Make sure the current GCode is loaded
        if (line < this.gcode_offset ||
            this.gcode_offset + this.gcode.length <= line) {
          this.gcode_offset = line - pageSize;
          if (this.gcode_offset < 0) this.gcode_offset = 0;

          this.gcode = this.all_gcode.slice(this.gcode_offset, maxLines);
        }

        Vue.nextTick(function () {
          var e = $('#gcode-line-' + line);
          if (e.length)
            e.addClass('highlight')[0]
            .scrollIntoView({behavior: 'smooth'});

          this.last_line = line;
        }.bind(this));
      }
    },


    update: function () {
      // Update file list
      api.get('file').done(function (files) {this.files = files}.bind(this))
    },


    submit_mdi: function () {
      this.send(this.mdi);
      if (!this.history.length || this.history[0] != this.mdi)
        this.history.unshift(this.mdi);
      this.mdi = '';
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
          file.name;
          if (file.name == this.last_file) this.last_file = '';
          this.update();
        }.bind(this));
    },


    load: function () {
      var file = this.state.selected;
      if (file == this.last_file) return;

      api.get('file/' + file)
        .done(function (data) {
          this.all_gcode = data.trimRight().split(/\r?\n/);
          this.gcode = this.all_gcode.slice(0, maxLines);
          this.gcode_offset = 0;
          this.last_file = file;

          Vue.nextTick(this.update_gcode_line);
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


    zero_axis: function (axis) {this.set_position(axis, 0)},


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
