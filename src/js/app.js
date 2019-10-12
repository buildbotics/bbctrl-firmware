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
var cookie = require('./cookie')('bbctrl-');
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


function is_object(o) {return o !== null && typeof o == 'object'}
function is_array(o) {return Array.isArray(o)}


function update_array(dst, src) {
  while (dst.length) dst.pop()
  for (var i = 0; i < src.length; i++)
    Vue.set(dst, i, src[i]);
}


function update_object(dst, src, remove) {
  var props, index, key, value;

  if (remove) {
    props = Object.getOwnPropertyNames(dst);

    for (index in props) {
      key = props[index];
      if (!src.hasOwnProperty(key))
        Vue.delete(dst, key);
    }
  }

  props = Object.getOwnPropertyNames(src);
  for (index in props) {
    key = props[index];
    value = src[key];

    if (is_array(value) && dst.hasOwnProperty(key) && is_array(dst[key]))
      update_array(dst[key], value);

    else if (is_object(value) && dst.hasOwnProperty(key) && is_object(dst[key]))
      update_object(dst[key], value, remove);

    else Vue.set(dst, key, value);
  }
}


module.exports = new Vue({
  el: 'body',


  data: function () {
    return {
      status: 'connecting',
      currentView: 'loading',
      index: -1,
      modified: false,
      template: require('../resources/config-template.json'),
      config: {
        settings: {units: 'METRIC'},
        motors: [{}, {}, {}, {}],
        version: '<loading>'
      },
      state: {messages: []},
      video_size: cookie.get('video-size', 'small'),
      crosshair: cookie.get('crosshair', false),
      errorTimeout: 30,
      errorTimeoutStart: 0,
      errorShow: false,
      errorMessage: '',
      confirmUpgrade: false,
      confirmUpload: false,
      firmwareUpgrading: false,
      checkedUpgrade: false,
      firmwareName: '',
      latestVersion: '',
      password: ''
    }
  },


  components: {
    'estop': {template: '#estop-template'},
    'loading-view': {template: '<h1>Loading...</h1>'},
    'control-view': require('./control-view'),
    'settings-view': require('./settings-view'),
    'motor-view': require('./motor-view'),
    'tool-view': require('./tool-view'),
    'io-view': require('./io-view'),
    'admin-general-view': require('./admin-general-view'),
    'admin-network-view': require('./admin-network-view'),
    'help-view': {template: '#help-view-template'},
    'cheat-sheet-view': {
      template: '#cheat-sheet-view-template',
      data: function () {return {showUnimplemented: false}}
    }
  },


  events: {
    'config-changed': function () {this.modified = true;},
    'hostname-changed': function (hostname) {this.hostname = hostname},


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


    upload: function (firmware) {
      this.firmware = firmware;
      this.firmwareName = firmware.name;
      this.password = '';
      this.confirmUpload = true;
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
    metric: function () {return this.config.settings.units != 'IMPERIAL'},


    block_error_dialog: function () {
      this.errorTimeoutStart = Date.now();
      this.errorShow = false;
    },


    toggle_video: function (e) {
      if      (this.video_size == 'small')  this.video_size = 'large';
      else if (this.video_size == 'large')  this.video_size = 'small';
      cookie.set('video-size', this.video_size);
    },


    toggle_crosshair: function (e) {
      e.preventDefault();
      this.crosshair = !this.crosshair;
      cookie.set('crosshair', this.crosshair);
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
        api.alert('Invalid password');
      }.bind(this))
    },


    upload_confirmed: function () {
      this.confirmUpload = false;

      var form = new FormData();
      form.append('firmware', this.firmware);
      if (this.password) form.append('password', this.password);

      $.ajax({
        url: '/api/firmware/update',
        type: 'PUT',
        data: form,
        cache: false,
        contentType: false,
        processData: false

      }).success(function () {
        this.firmwareUpgrading = true;

      }.bind(this)).error(function () {
        api.alert('Invalid password or bad firmware');
      }.bind(this))
    },


    show_upgrade: function () {
      if (!this.latestVersion) return false;
      return compare_versions(this.config.version, this.latestVersion) < 0;
    },


    update: function () {
      api.get('config/load').done(function (config) {
        update_object(this.config, config, true);
        this.parse_hash();

        if (!this.checkedUpgrade) {
          this.checkedUpgrade = true;

          var check = this.config.admin['auto-check-upgrade'];
          if (typeof check == 'undefined' || check)
            this.$emit('check');
        }
      }.bind(this))
    },


    connect: function () {
      this.sock = new Sock('//' + window.location.host + '/sockjs');

      this.sock.onmessage = function (e) {
        if (typeof e.data != 'object') return;

        if ('log' in e.data) {
          this.$broadcast('log', e.data.log);
          delete e.data.log;
        }

        // Check for session ID change on controller
        if ('sid' in e.data) {
          if (typeof this.sid == 'undefined') this.sid = e.data.sid;

          else if (this.sid != e.data.sid) {
            if (typeof this.hostname != 'undefined' &&
                String(location.hostname) != 'localhost')
              location.hostname = this.hostname;
            location.reload(true);
          }
        }

        update_object(this.state, e.data, false);
        this.$broadcast('update');

      }.bind(this)

      this.sock.onopen = function (e) {
        this.status = 'connected';
        this.$emit(this.status);
        this.$broadcast(this.status);
      }.bind(this)

      this.sock.onclose = function (e) {
        this.status = 'disconnected';
        this.$emit(this.status);
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
        api.alert('Save failed', error);
      });
    },


    close_messages: function (action) {
      if (action == 'stop') api.put('stop');
      if (action == 'continue') api.put('unpause');

      // Acknowledge messages
      if (this.state.messages.length) {
        var id = this.state.messages.slice(-1)[0].id
        api.put('message/' + id + '/ack');
      }
    }
  }
})
