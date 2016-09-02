'use strict'

var api = require('./api');


function is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
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
      gcode: '',
      speed_override: 1,
      feed_override: 1
    }
  },


  components: {
    'axis-control': require('./axis-control'),
    'estop': {template: '#estop-template'}
  },


  events: {
    jog: function (axis, move) {this.send('g91 g0' + axis + move)},
    home: function (axis) {this.send('$home ' + axis)},
    zero: function (axis) {this.send('$zero ' + axis)}
  },


  ready: function () {
    this.update();
  },


  methods: {
    send: function (msg) {
      this.$dispatch('send', msg);
    },


    enabled: function (axis) {
      var axis = axis.toLowerCase();
      return axis in this.config.axes &&
        this.config.axes[axis].mode != 'disabled';
    },


    estop: function () {
      if (this.state.x == 'estopped') api.put('clear').done(this.update);
      else api.put('estop').done(this.update);
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
    },


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
        this.gcode = '';
        return;
      }

      if (file == this.last_file) return;

      api.get('file/' + file)
        .done(function (data) {
          this.gcode = data;
          this.last_file = file;
        }.bind(this));
    },


    delete: function () {
      if (!this.file) return;
      api.delete('file/' + this.file).done(this.update);
    },


    home: function () {api.put('home').done(this.update)},


    start_pause: function () {
      if (this.state.x == 'running') this.pause();
      else this.start();
    },


    start: function () {api.put('start').done(this.update)},
    pause: function () {api.put('pause').done(this.update)},
    optional_pause: function () {api.put('pause/optional').done(this.update)},
    stop: function () {api.put('stop').done(this.update)},
    step: function () {api.put('step').done(this.update)},


    override_feed: function () {
      api.put('override/feed/' + this.feed_override).done(this.update)
    },


    override_speed: function () {
      api.put('override/speed/' + this.speed_override).done(this.update)
    },


    current: function (axis, value) {
      var x = value / 32.0;
      if (this.state[axis + 'pl'] == x) return;

      var data = {};
      data[axis + 'pl'] = x;
      this.send(JSON.stringify(data));
    }
  },


  filters: {
    percent: function (value, precision) {
      if (typeof precision == 'undefined') precision = 2;
      return (value * 100.0).toFixed(precision) + '%';
    },


    fixed: function (value, precision) {
      return value.toFixed(precision);
    }
  }
}
