'use strict'


function is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]';
}


module.exports = {
  template: '#status-view-template',


  data: function () {
    return {
      axes: 'xyza',
      state: {
        xpl: 1, ypl: 1, zpl: 1, apl: 1
      },
      step: 10
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
  },


  methods: {
    send: function (data) {
      this.sock.send(JSON.stringify(data));
    },


    jog: function (axis, dir) {
      this.sock.send('g91 g0' + axis + (dir * this.step));
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
