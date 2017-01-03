'use strict'

var api = require('./api');
var Sock = require('./sock');


module.exports = new Vue({
  el: 'body',


  data: function () {
    return {
      status: 'connecting',
      currentView: 'loading',
      index: -1,
      modified: false,
      template: {motors: {}, axes: {}},
      config: {motors: [{}]},
      state: {}
    }
  },


  components: {
    'estop': {template: '#estop-template'},
    'loading-view': {template: '<h1>Loading...</h1>'},
    'control-view': require('./control-view'),
    'motor-view': require('./motor-view'),
    'spindle-view': require('./spindle-view'),
    'switches-view': require('./switches-view'),
    'gcode-view': require('./gcode-view'),
    'admin-view': require('./admin-view')
  },


  events: {
    'config-changed': function () {this.modified = true;},


    send: function (msg) {
      if (this.status == 'connected') {
        console.debug('>', msg);
        this.sock.send(msg)
      }
    },


    connected: function () {this.update()},
    update: function () {this.update()}
  },


  ready: function () {
    $(window).on('hashchange', this.parse_hash);
    this.connect();
  },


  methods: {
    estop: function () {
      if (this.state.x == 'ESTOPPED') api.put('clear');
      else api.put('estop');
    },


    update: function () {
      $.ajax({type: 'GET', url: '/config-template.json', cache: false})
        .success(function (data, status, xhr) {
          this.template = data;

          api.get('config/load').done(function (data) {
            this.config = data;
            this.parse_hash();
          }.bind(this))
        }.bind(this))
    },


    connect: function () {
      this.sock = new Sock('//' + window.location.host + '/ws');

      this.sock.onmessage = function (e) {
        var msg = e.data;

        if (typeof msg == 'object') {
          for (var key in msg)
            this.$set('state.' + key, msg[key]);

          if ('msg' in msg) this.$broadcast('message', msg);
        }
      }.bind(this);

      this.sock.onopen = function (e) {
        this.status = 'connected';
        this.$emit(this.status);
        this.$broadcast(this.status);
      }.bind(this);

      this.sock.onclose = function (e) {
        this.status = 'disconnected';
        this.$broadcast(this.status);
      }.bind(this);
    },


    parse_hash: function () {
      var hash = location.hash.substr(1);

      if (!hash.trim().length) {
        location.hash = 'control';
        return;
      }

      var parts = hash.split(':');

      if (parts.length == 2) this.index = parts[1];

      this.currentView = parts[0];
    },


    save: function () {
      api.put('config/save', this.config).done(function (data) {
        this.modified = false;
      }.bind(this)).fail(function (error) {
        alert('Save failed: ' + error);
      });
    }
  }
})
