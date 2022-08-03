/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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
var cookie = require('./cookie');
var util   = require('./util');


function _is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
}


function escapeHTML(s) {
  var entityMap = {'&': '&amp;', '<': '&lt;', '>': '&gt;'};
  return String(s).replace(/[&<>]/g, function (s) {return entityMap[s];});
}


module.exports = {
  template: '#view-control-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      mach_units: 'METRIC',
      mdi: '',
      axes: 'xyzabc',
      history: [],
      manual_home: {x: false, y: false, z: false, a: false, b: false, c: false},
      position_msg:
      {x: false, y: false, z: false, a: false, b: false, c: false},
      axis_position: 0,
      jog_step: cookie.get_bool('jog-step'),
      jog_adjust: parseInt(cookie.get('jog-adjust', 2)),
      tab: 'auto',
      highlighted_line: 0,
      toolpath: {}
    }
  },


  components: {
    'axis-control': require('./axis-control')
  },


  watch: {
    'state.imperial': {
      handler(imperial) {
        this.mach_units = imperial ? 'IMPERIAL' : 'METRIC';
      },
      immediate: true
    },


    mach_units(units) {
      if ((units == 'METRIC') != this.metric)
        this.send(units == 'METRIC' ? 'G21' : 'G20');
    },


    'state.line'() {
      if (this.mach_state != 'HOMING') this.highlight_code();
    },


    'active.path'() {this.load()},
    jog_step() {cookie.set_bool('jog-step', this.jog_step)},
    jog_adjust() {cookie.set('jog-adjust', this.jog_adjust)}
  },


  computed: {
    active() {
      return this.$root.active_program || this.$root.selected_program
    },


    metric() {return !this.state.imperial},


    mach_state() {
      var cycle = this.state.cycle;
      var state = this.state.xx;

      if (typeof cycle != 'undefined' && state != 'ESTOPPED' &&
          (cycle == 'jogging' || cycle == 'homing'))
        return cycle.toUpperCase();
      return state || ''
    },


    pause_reason() {return this.state.pr},


    is_running() {
      return this.mach_state == 'RUNNING' || this.mach_state == 'HOMING';
    },


    is_stopping() {return this.mach_state  == 'STOPPING'},
    is_holding()  {return this.mach_state  == 'HOLDING'},
    is_ready()    {return this.mach_state  == 'READY'},
    is_idle()     {return this.state.cycle == 'idle'},


    is_paused() {
      return this.is_holding &&
        (this.pause_reason == 'User pause' ||
         this.pause_reason == 'Program pause')
    },


    can_mdi() {return this.is_idle || this.state.cycle == 'mdi'},


    can_set_axis() {
      return this.is_idle
      // TODO allow setting axis position during pause
      //   return this.is_idle || this.is_paused
    },


    message() {
      if (this.mach_state == 'ESTOPPED') return this.state.er;
      if (this.mach_state == 'HOLDING') return this.state.pr;
      if (this.state.messages.length)
        return this.state.messages.slice(-1)[0].text;
      return '';
    },


    highlight_state() {
      return this.mach_state == 'ESTOPPED' || this.mach_state == 'HOLDING';
    },


    plan_time() {return this.state.plan_time},


    remaining() {
      if (!(this.is_stopping || this.is_running || this.is_holding)) return 0;
      if (this.active.time < this.plan_time) return 0;
      return this.active.time - this.plan_time
    },


    eta() {
      if (this.mach_state != 'RUNNING') return '';
      var d = new Date();
      d.setSeconds(d.getSeconds() + this.remaining);
      return d.toLocaleString();
    },


    simulating() {
      return 0 < this.active.progress && this.active.progress < 1
    },


    progress() {
      if (this.simulating) return this.active.progress;

      if (!this.active.time || this.is_ready) return 0;
      var p = this.plan_time / this.active.time;
      return p < 1 ? p : 1;
    }
  },


  events: {
    jog(axis, power) {
      var data = {ts: new Date().getTime()};
      data[axis] = power;
      api.put('jog', data);
    },


    step(axis, value) {
      this.send('M70\nG91\nG0' + axis + value + '\nM72');
    }
  },


  ready() {
    this.editor = CodeMirror.fromTextArea(this.$els.gcodeView, {
      readOnly: true,
      lineNumbers: true,
      mode: 'gcode'
    })

    this.editor.on('scrollCursorIntoView', this.on_scroll);
    this.load()
  },


  attached() {if (this.editor) this.editor.refresh()},


  methods: {
    // From axis-vars
    get_bounds() {return this.toolpath.bounds},


    goto(hash) {window.location.hash = hash},
    send(msg) {this.$dispatch('send', msg)},
    on_scroll(cm, e) {e.preventDefault()},


    run_macro(macro) {
      api.put('macro/' + macro)
        .fail((error) => {
          this.$root.error_dialog(
            'Failed to run macro "' + macro + '":\n' + error.message)
        })
    },


    highlight_code() {
      if (typeof this.editor == 'undefined') return;
      var line = this.state.line - 1;
      var doc = this.editor.getDoc();

      doc.removeLineClass(this.highlighted_line, 'wrap', 'highlight');

      if (0 <= line) {
        doc.addLineClass(line, 'wrap', 'highlight');
        this.highlighted_line = line;
        this.editor.scrollIntoView({line: line, ch: 0}, 200);
      }
    },


    load() {
      let path = this.active.path
      if (!path) return

      this.active.load()
        .done((data) => {
          if (this.active.path != path) return

          this.editor.setOption('mode', util.get_highlight_mode(path))
          this.editor.setValue(data)
          this.highlight_code()
        })

      this.active.toolpath()
        .done((data) => {
          if (this.active.path != path) return
          this.toolpath = data
        })
    },


    submit_mdi() {
      this.send(this.mdi);
      if (!this.history.length || this.history[0] != this.mdi)
        this.history.unshift(this.mdi);
      this.mdi = '';
    },


    mdi_start_pause() {
      if (this.state.xx == 'RUNNING') this.pause();

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause();

      else this.submit_mdi();
    },


    load_history(index) {this.mdi = this.history[index];},


    home(axis) {
      if (typeof axis == 'undefined') api.put('home');

      else {
        if (this[axis].homingMode != 'manual') api.put('home/' + axis);
        else this.manual_home[axis] = true;
      }
    },


    set_home(axis, position) {
      this.manual_home[axis] = false;
      api.put('home/' + axis + '/set', {position: parseFloat(position)});
    },


    unhome(axis) {
      this.position_msg[axis] = false;
      api.put('home/' + axis + '/clear');
    },


    show_set_position(axis) {
      this.axis_position = 0;
      this.position_msg[axis] = true;
    },


    set_position(axis, position) {
      this.position_msg[axis] = false;
      api.put('position/' + axis, {'position': parseFloat(position)});
    },


    zero_all() {
      for (var axis of 'xyzabc')
        if (this[axis].enabled) this.zero(axis);
    },


    zero(axis) {
      if (typeof axis == 'undefined') this.zero_all();
      else this.set_position(axis, 0);
    },


    start_pause() {
      if (this.state.xx == 'RUNNING') this.pause();

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause();

      else this.start();
    },


    start()          {api.put('start/' + this.$root.selected_program.path)},
    pause()          {api.put('pause')},
    unpause()        {api.put('unpause')},
    optional_pause() {api.put('pause/optional')},
    stop()           {api.put('stop')},
    step()           {api.put('step')},


    open() {
      let path = this.active.path

      this.$root.file_dialog({
        callback: (path) => {if (path) this.$root.select_path(path)},
        dir: path ? util.dirname(path) : '/'
      })
    },


    edit() {this.$root.edit(this.active.path)},
    view() {this.$root.view(this.active.path)},


    current(axis, value) {
      var x = value / 32.0;
      if (this.state[axis + 'pl'] == x) return;

      var data = {};
      data[axis + 'pl'] = x;
      this.send(JSON.stringify(data));
    }
  },


  mixins: [require('./axis-vars')]
}
