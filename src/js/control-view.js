'use strict'

var api = require('./api');


function is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
}


module.exports = {
  template: '#control-view-template',
  props: ['config'],


  data: function () {
    return {
      mdi: '',
      file: '',
      last_file: '',
      files: [],
      axes: 'xyzabc',
      state: {},
      gcode: '',
      speed_override: 0,
      feed_override: 0
    }
  },


  components: {
    'axis-control': require('./axis-control'),
    'estop': {template: '#estop-template'}
  },


  events: {
    jog: function (axis, move) {
      console.debug('jog(' + axis + ', ' + move + ')');
      this.send('g91 g0' + axis + move);
    },


    home: function (axis) {
      console.debug('home(' + axis + ')');
      this.send('$home ' + axis);
    },


    zero: function (axis) {
      console.debug('zero(' + axis + ')');
      this.send('$zero ' + axis);
    },


    message: function (data) {
      if (typeof data == 'object')
        for (var key in data)
          this.$set('state.' + key, data[key]);
    }
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
      this.$set('state.es', !this.state.es);
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


    run: function (file) {
      api.put('file/' + file).done(this.update);
    },


    current: function (axis, value) {
      var x = value / 32.0;
      if (this.state[axis + 'pl'] == x) return;

      var data = {};
      data[axis + 'pl'] = x;
      this.send(JSON.stringify(data));
    },


    override_feed: function () {},
    override_speed: function () {},
    step: function () {},
    stop: function () {},
    optional_stop: function () {},


    home: function () {
      this.send('$calibrate');
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
