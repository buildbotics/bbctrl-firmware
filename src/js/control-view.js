/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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

'use strict'

var api    = require('./api');
var cookie = require('./cookie')('bbctrl-');


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
      mach_units: 'METRIC',
      mdi: '',
      last_file: undefined,
      last_file_time: undefined,
      toolpath: {},
      toolpath_progress: 0,
      axes: 'xyzabc',
      history: [],
      speed_override: 1,
      feed_override: 1,
      manual_home: {x: false, y: false, z: false, a: false, b: false, c: false},
      position_msg:
      {x: false, y: false, z: false, a: false, b: false, c: false},
      axis_position: 0,
      jog_step: cookie.get_bool('jog-step'),
      jog_adjust: parseInt(cookie.get('jog-adjust', 2)),
      deleteGCode: false,
      tab: 'auto'
    }
  },


  components: {
    'axis-control': require('./axis-control'),
    'path-viewer': require('./path-viewer'),
    'gcode-viewer': require('./gcode-viewer')
  },


  watch: {
    'state.imperial': {
      handler: function (imperial) {
        this.mach_units = imperial ? 'IMPERIAL' : 'METRIC';
      },
      immediate: true
    },


    mach_units: function (units) {
      if ((units == 'METRIC') != this.metric)
        this.send(units == 'METRIC' ? 'G21' : 'G20');
    },


    'state.line': function () {
      if (this.mach_state != 'HOMING')
        this.$broadcast('gcode-line', this.state.line);
    },


    'state.selected_time': function () {this.load()},
    jog_step: function () {cookie.set_bool('jog-step', this.jog_step)},
    jog_adjust: function () {cookie.set('jog-adjust', this.jog_adjust)}
  },


  computed: {
    metric: function () {return !this.state.imperial},


    mach_state: function () {
      var cycle = this.state.cycle;
      var state = this.state.xx;

      if (typeof cycle != 'undefined' && state != 'ESTOPPED' &&
          (cycle == 'jogging' || cycle == 'homing'))
        return cycle.toUpperCase();
      return state || ''
    },


    pause_reason: function () {return this.state.pr},


    is_running: function () {
      return this.mach_state == 'RUNNING' || this.mach_state == 'HOMING';
    },


    is_stopping: function () {return this.mach_state == 'STOPPING'},
    is_holding: function () {return this.mach_state == 'HOLDING'},
    is_ready: function () {return this.mach_state == 'READY'},
    is_idle: function () {return this.state.cycle == 'idle'},


    is_paused: function () {
      return this.is_holding &&
        (this.pause_reason == 'User pause' ||
         this.pause_reason == 'Program pause')
    },


    can_mdi: function () {return this.is_idle || this.state.cycle == 'mdi'},


    can_set_axis: function () {
      return this.is_idle
      // TODO allow setting axis position during pause
      return this.is_idle || this.is_paused
    },


    message: function () {
      if (this.mach_state == 'ESTOPPED') return this.state.er;
      if (this.mach_state == 'HOLDING') return this.state.pr;
      if (this.state.messages.length)
        return this.state.messages.slice(-1)[0].text;
      return '';
    },


    highlight_state: function () {
      return this.mach_state == 'ESTOPPED' || this.mach_state == 'HOLDING';
    },


    plan_time: function () {return this.state.plan_time},


    plan_time_remaining: function () {
      if (!(this.is_stopping || this.is_running || this.is_holding)) return 0;
      return this.toolpath.time - this.plan_time
    },


    eta: function () {
      if (this.mach_state != 'RUNNING') return '';
      var remaining = this.plan_time_remaining;
      var d = new Date();
      d.setSeconds(d.getSeconds() + remaining);
      return d.toLocaleString();
    },


    progress: function () {
      if (!this.toolpath.time || this.is_ready) return 0;
      var p = this.plan_time / this.toolpath.time;
      return p < 1 ? p : 1;
    }
  },


  events: {
    jog: function (axis, power) {
      var data = {ts: new Date().getTime()};
      data[axis] = power;
      api.put('jog', data);
    },


    step: function (axis, value) {
      this.send('M70\nG91\nG0' + axis + value + '\nM72');
    }
  },


  ready: function () {this.load()},


  methods: {
    send: function (msg) {this.$dispatch('send', msg)},


    load: function () {
      var file_time = this.state.selected_time;
      var file = this.state.selected;
      if (this.last_file == file && this.last_file_time == file_time) return;
      this.last_file = file;
      this.last_file_time = file_time;

      this.$broadcast('gcode-load', file);
      this.$broadcast('gcode-line', this.state.line);
      this.toolpath_progress = 0;
      this.load_toolpath(file, file_time);
    },


    load_toolpath: function (file, file_time) {
      this.toolpath = {};

      if (!file) return;

      api.get('path/' + file).done(function (toolpath) {
        if (this.last_file_time != file_time) return;

        if (typeof toolpath.progress == 'undefined') {
          toolpath.filename = file;
          this.toolpath_progress = 1;
          this.toolpath = toolpath;

          var state = this.$root.state;
          var bounds = toolpath.bounds;
          for (var axis of 'xyzabc') {
            Vue.set(state, 'path_min_' + axis, bounds.min[axis]);
            Vue.set(state, 'path_max_' + axis, bounds.max[axis]);
          }

        } else {
          this.toolpath_progress = toolpath.progress;
          this.load_toolpath(file, file_time); // Try again
        }
      }.bind(this));
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
      // If we don't reset the form the browser may cache file if name is same
      // even if contents have changed
      $('.gcode-file-input')[0].reset();
      $('.gcode-file-input input').click();
    },


    upload: function (e) {
      var files = e.target.files || e.dataTransfer.files;
      if (!files.length) return;

      var file = files[0];
      var fd = new FormData();

      fd.append('gcode', file);

      api.upload('file', fd)
        .done(function () {
          this.last_file_time = undefined; // Force reload
          this.$broadcast('gcode-reload', file.name);

        }.bind(this)).fail(function (error) {
          api.alert('Upload failed', error)
        }.bind(this));
    },


    delete_current: function () {
      if (this.state.selected)
        api.delete('file/' + this.state.selected);
      this.deleteGCode = false;
    },


    delete_all: function () {
      api.delete('file');
      this.deleteGCode = false;
    },


    home: function (axis) {
      if (typeof axis == 'undefined') api.put('home');

      else {
        if (this[axis].homingMode != 'manual') api.put('home/' + axis);
        else this.manual_home[axis] = true;
      }
    },


    set_home: function (axis, position) {
      this.manual_home[axis] = false;
      api.put('home/' + axis + '/set', {position: parseFloat(position)});
    },


    unhome: function (axis) {
      this.position_msg[axis] = false;
      api.put('home/' + axis + '/clear');
    },


    show_set_position: function (axis) {
      this.axis_position = 0;
      this.position_msg[axis] = true;
    },


    set_position: function (axis, position) {
      this.position_msg[axis] = false;
      api.put('position/' + axis, {'position': parseFloat(position)});
    },


    zero_all: function () {
      for (var axis of 'xyzabc')
        if (this[axis].enabled) this.zero(axis);
    },


    zero: function (axis) {
      if (typeof axis == 'undefined') this.zero_all();
      else this.set_position(axis, 0);
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
    }
  },


  mixins: [require('./axis-vars')]
}
