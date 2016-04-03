'use strict'

var api = require('./api');


function is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
}


module.exports = {
  template: '#status-view-template',


  data: function () {
    return {
      mdi: '',
      uploads: [],
      axes: 'xyza',
      state: {xpl: 1, ypl: 1, zpl: 1, apl: 1}
    }
  },


  components: {
    'axis-control': require('./axis-control')
  },


  events: {
    jog: function (axis, move) {
      console.debug('jog(' + axis + ', ' + move + ')');
      this.sock.send('g91 g0' + axis + move);
    },


    home: function (axis) {
      console.debug('home(' + axis + ')');
      this.sock.send('$home ' + axis);
    },


    zero: function (axis) {
      console.debug('zero(' + axis + ')');
      this.sock.send('$zero ' + axis);
    }
  },


  ready: function () {
    this.sock = new SockJS('//' + window.location.host + '/ws');

    this.sock.onmessage = function (e) {
      var data = e.data;
      console.debug(data);

      for (var key in data) {
        this.$set('state.' + key, data[key]);

        for (var axis of ['x', 'y', 'z', 'a'])
          if (key == axis + 'pl' &&
              typeof this.$get('current' + axis) == 'undefined')
            this.$set('current' + axis, (32 * data[key]).toFixed());
      }
    }.bind(this);

    this.update();
  },


  methods: {
    update: function () {
      api.get('upload')
        .done(function (uploads) {
          this.uploads = uploads;
        }.bind(this))
    },


    submit_mdi: function () {
      this.sock.send(this.mdi);
    },


    upload: function (e) {
      var files = e.target.files || e.dataTransfer.files;
      if (!files.length) return;

      var fd = new FormData();
      fd.append('gcode', files[0]);

      api.upload('upload', fd).done(this.update);
    },


    delete: function (file) {
      api.delete('upload/' + file).done(this.update);
    },


    run: function (file) {
      api.put('upload/' + file).done(this.update);
    },


    send: function (data) {
      this.sock.send(JSON.stringify(data));
    },


    current: function (axis, value) {
      var x = value / 32.0;
      if (this.state[axis + 'pl'] == x) return;

      var data = {};
      data[axis + 'pl'] = x;
      this.send(data);
    }
  },


  filters: {
    percent: function (value, precision) {
      if (typeof precision == 'undefined') precision = 2;
      return (value * 100.0).toFixed(precision) + '%';
    }
  }
}
