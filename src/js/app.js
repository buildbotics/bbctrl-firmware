'use strict'

var api = require('./api');
var Sock = require('./sock');


function compare_versions(a, b) {
    var reStripTrailingZeros = /(\.0+)+$/;
    var segsA = a.replace(reStripTrailingZeros, '').split('.');
    var segsB = b.replace(reStripTrailingZeros, '').split('.');
    var l = Math.min(segsA.length, segsB.length);

    for (var i = 0; i < l; i++) {
      var diff = parseInt(segsA[i], 10) - parseInt(segsB[i], 10);
      if (diff) return diff;
    }

    return segsA.length - segsB.length;
}


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
      state: {},
      messages: [],
      errorShow: false,
      errorMessage: '',
      confirmUpgrade: false,
      firmwareUpgrading: false,
      checkedUpgrade: false,
      latestVersion: '',
      password: ''
    }
  },


  components: {
    'estop': {template: '#estop-template'},
    'loading-view': {template: '<h1>Loading...</h1>'},
    'control-view': require('./control-view'),
    'motor-view': require('./motor-view'),
    'tool-view': require('./tool-view'),
    'io-view': require('./io-view'),
    'gcode-view': require('./gcode-view'),
    'admin-view': require('./admin-view')
  },


  events: {
    'config-changed': function () {this.modified = true;},


    send: function (msg) {
      if (this.status == 'connected') {
        console.debug('>', msg);
        this.sock.send(msg);
      }
    },


    connected: function () {this.update()},
    update: function () {this.update()},


    check: function () {
      this.latestVersion = '';

      $.ajax({
        type: 'GET',
        url: 'https://buildbotics.com/bbctrl/latest.txt',
        data: {hid: this.state.hid},
        cache: false

      }).done(function (data) {
        this.latestVersion = data;
        this.$broadcast('latest_version', data);
      }.bind(this))
    },


    upgrade: function () {
      this.password = '';
      this.confirmUpgrade = true;
    },


    error: function (msg) {
      this.errorShow = true;
      this.errorMessage = msg.msg;
    }
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


    upgrade_confirmed: function () {
      this.confirmUpgrade = false;

      api.put('upgrade', {password: this.password}).done(function () {
        this.firmwareUpgrading = true;

      }.bind(this)).fail(function () {
        alert('Invalid password');
      }.bind(this))
    },


    show_upgrade: function () {
      if (!this.latestVersion) return false;
      return compare_versions(this.config.version, this.latestVersion) < 0;
    },


    update: function () {
      $.ajax({type: 'GET', url: '/config-template.json', cache: false})
        .success(function (data, status, xhr) {
          this.template = data;

          api.get('config/load').done(function (data) {
            this.config = data;
            this.parse_hash();

            if (!this.checkedUpgrade) {
              this.checkedUpgrade = true;

              var check = this.config.admin['auto-check-upgrade'];
              if (typeof check == 'undefined' || check)
                this.$emit('check');
            }
          }.bind(this))
        }.bind(this))
    },


    connect: function () {
      this.sock = new Sock('//' + window.location.host + '/sockjs');

      this.sock.onmessage = function (e) {
        var msg = e.data;

        if (typeof msg == 'object')
          for (var key in msg) {
            if (key == 'msg') this.$broadcast('message', msg.msg);
            else Vue.set(this.state, key, msg[key]);
          }
      }.bind(this)

      this.sock.onopen = function (e) {
        this.status = 'connected';
        this.$emit(this.status);
        this.$broadcast(this.status);
      }.bind(this)

      this.sock.onclose = function (e) {
        this.status = 'disconnected';
        this.$broadcast(this.status);
      }.bind(this)
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
