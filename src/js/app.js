/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

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
      errorTimeout: 30,
      errorTimeoutStart: 0,
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
      // Honor user error blocking
      if (Date.now() - this.errorTimeoutStart < this.errorTimeout * 1000)
        return;

      // Wait at least 1 sec to pop up repeated errors
      if (1 < msg.repeat && Date.now() - msg.ts < 1000) return;

      // Popup error dialog
      this.errorShow = true;
      this.errorMessage = msg.msg;
    }
  },


  ready: function () {
    $(window).on('hashchange', this.parse_hash);
    this.connect();
  },


  methods: {
    block_error_dialog: function () {
      this.errorTimeoutStart = Date.now();
      this.errorShow = false;
    },


    estop: function () {
      if (this.state.xx == 'ESTOPPED') api.put('clear');
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
