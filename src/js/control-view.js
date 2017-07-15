'use strict'

var api = require('./api');


function _is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
}


function _msg_equal(a, b) {
  return a.level == b.level && a.location == b.location && a.code == b.code &&
    a.msg == b.msg;
}


function escapeHTML(s) {
  var entityMap = {'&': '&amp;', '<': '&lt;', '>': '&gt;'};
  return String(s).replace(/[&<>]/g, function (s) {return entityMap[s];});
}


module.exports = {
  template: '#control-view-template',
  props: ['config', 'state'],


  data: function () {
    return {
      mdi: '',
      file: '',
      last_file: '',
      files: [],
      axes: 'xyzabc',
      gcode: [],
      history: [],
      console: [],
      speed_override: 1,
      feed_override: 1,
      manual_home: {x: false, y: false, z: false, a: false, b: false, c: false},
      position_msg:
      {x: false, y: false, z: false, a: false, b: false, c: false},
      axis_position: 0,
      video_url: ''
    }
  },


  components: {
    'axis-control': require('./axis-control')
  },


  watch: {
    'state.ln': function () {this.update_gcode_line();}
  },


  events: {
    // TODO These should all be implemented via the API
    jog: function (axis, move) {this.send('g91 g0' + axis + move)},


    message: function (msg) {
      if (this.console.length &&
          _msg_equal(msg, this.console[this.console.length - 1]))
        this.console[this.console.length - 1].repeat++;

      else {
        msg.repeat = 1;
        this.console.push(msg);
      }
    }
  },


  ready: function () {
    this.update();
  },


  methods: {
    get_state: function () {
      var state = this.state.x || '';
      if (state == 'RUNNING' && this.state.c) state = this.state.c;
      return state;
    },


    get_reason: function () {
      if (this.state.x == 'ESTOPPED') return this.state.er;
      if (this.state.x == 'HOLDING') return this.state.pr;
      return '';
    },


    send: function (msg) {
      this.$dispatch('send', msg);
    },


    get_axis_motor_id: function (axis) {
      var axis = axis.toLowerCase();

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


    enabled: function (axis) {
      var motor = this.get_axis_motor(axis);
      return typeof motor != 'undefined' && motor['power-mode'] != 'disabled';
    },


    is_homed: function (axis) {
      var motor = this.get_axis_motor_id(axis);
      return motor != -1 && this.state[motor + 'h'];
    },


    update_gcode_line: function () {
      if (typeof this.last_line != 'undefined') {
        $('#gcode-line-' + this.last_line).removeClass('highlight');
        this.last_line = undefined;
      }

      if (0 <= this.state.ln) {
        var line = this.state.ln - 1;
        var e = $('#gcode-line-' + line);
        if (e.length)
          e.addClass('highlight')[0]
          .scrollIntoView({behavior: 'smooth'});

        this.last_line = line;
      }
    },


    update: function () {
      api.get('file')
        .done(function (files) {
          var index = files.indexOf(this.file);
          if (index == -1 && files.length) this.file = files[0];

          this.files = files;

          this.load()
        }.bind(this))
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
          this.file = file.name;
          if (file.name == this.last_file) this.last_file = '';
          this.update();
        }.bind(this));
    },


    load: function () {
      var file = this.file;

      if (!file || this.files.indexOf(file) == -1) {
        this.file = '';
        this.gcode = [];
        return;
      }

      if (file == this.last_file) return;

      api.get('file/' + file)
        .done(function (data) {
          this.gcode = data.trimRight().split(/\r?\n/);
          this.last_file = file;

          Vue.nextTick(this.update_gcode_line);
        }.bind(this));
    },


    delete: function () {
      if (!this.file) return;
      api.delete('file/' + this.file).done(this.update);
    },


    home: function (axis) {
      if (typeof axis == 'undefined') api.put('home');

      else {
        var motor = this.get_axis_motor(axis);

        if (motor['homing-mode'] == 'manual') {
          this.axis_position = this.state[axis + 'w'];
          this.manual_home[axis] = true;

        } else api.put('home/' + axis);
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


    get_offset: function (axis) {
      return this.state[axis + 'w'] - this.state[axis + 'p'];
    },


    set_position: function (axis, position) {
      this.position_msg[axis] = false;
      api.put('position/' + axis, {position: parseFloat(position)});
    },


    start_pause: function () {
      if (this.state.x == 'RUNNING') this.pause();

      else if (this.state.x == 'STOPPING' || this.state.x == 'HOLDING')
        this.unpause();

      else this.start();
    },


    start: function () {api.put('start/' + this.file).done(this.update)},
    pause: function () {api.put('pause')},
    unpause: function () {api.put('unpause')},
    optional_pause: function () {api.put('pause/optional')},
    stop: function () {api.put('stop')},
    step: function () {api.put('step/' + this.file).done(this.update)},


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


    clear_console: function () {this.console = [];},


    load_video: function () {
      this.video_url = '//' + document.location.hostname + ':8000/stream/0';
    }
  }
}
