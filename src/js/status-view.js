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
        dcurx: 1, dcury: 1, dcurz: 1, dcura: 1
        //sguardx: 1, sguardy: 1, sguardz: 1, sguarda: 1
      },
      step: 10
    }
  },


  components: {
    gauge: require('./gauge')
  },


  watch: {
    currentx: function (value) {this.current('x', value);},
    currenty: function (value) {this.current('y', value);},
    currentz: function (value) {this.current('z', value);},
    currenta: function (value) {this.current('a', value);}
  },


  ready: function () {
    this.sock = new SockJS('//' + window.location.host + '/ws');

    this.sock.onmessage = function (e) {
      var data = e.data;
      console.debug(data);

      for (var key in data) {
        this.$set('state.' + key, data[key]);

        for (var axis of ['x', 'y', 'z', 'a'])
          if (key == 'dcur' + axis &&
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
      var pos = this.state['pos' + axis] + dir * this.step;
      this.sock.send('g0' + axis + pos);
    },


    current: function (axis, value) {
      var x = value / 32.0;
      if (this.state['dcur' + axis] == x) return;

      var data = {};
      data['dcur' + axis] = x;
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
