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


module.exports = {
  template: '#admin-network-view-template',
  props: ['config', 'state'],


  data: function () {
    return {
      hostnameSet: false,
      usernameSet: false,
      passwordSet: false,
      redirectTimeout: 0,
      hostname: '',
      username: '',
      current: '',
      password: '',
      password2: '',
      wifi_mode: 'client',
      wifi_ssid: '',
      wifi_ch: undefined,
      wifi_pass: '',
      wifiConfirm: false,
      rebooting: false
    }
  },


  ready: function () {
    api.get('hostname').done(function (hostname) {
      this.hostname = hostname;
    }.bind(this));

    api.get('remote/username').done(function (username) {
      this.username = username;
    }.bind(this));

    api.get('wifi').done(function (config) {
      this.wifi_mode = config.mode;
      this.wifi_ssid = config.ssid;
      this.wifi_ch = config.channel;
    }.bind(this));
  },


  methods: {
    redirect: function (hostname) {
      if (0 < this.redirectTimeout) {
        this.redirectTimeout -= 1;
        setTimeout(function () {this.redirect(hostname)}.bind(this), 1000);

      } else location.hostname = hostname;
    },


    set_hostname: function () {
      api.put('hostname', {hostname: this.hostname}).done(function () {
        this.redirectTimeout = 45;
        this.hostnameSet = true;

        api.put('reboot').always(function () {
          var hostname = this.hostname;
          if (String(location.hostname).endsWith('.local'))
            hostname += '.local'
          this.$dispatch('hostname-changed', hostname);
          this.redirect(hostname);
        }.bind(this));

      }.bind(this)).fail(function (error) {
        alert('Set hostname failed: ' + JSON.stringify(error));
      })
    },


    set_username: function () {
      api.put('remote/username', {username: this.username}).done(function () {
        this.usernameSet = true;
      }.bind(this)).fail(function (error) {
        alert('Set username failed: ' + JSON.stringify(error));
      })
    },


    set_password: function () {
      if (this.password != this.password2) {
        alert('Passwords to not match');
        return;
      }

      if (this.password.length < 6) {
        alert('Password too short');
        return;
      }

      api.put('remote/password', {
        current: this.current,
        password: this.password
      }).done(function () {
        this.passwordSet = true;
      }.bind(this)).fail(function (error) {
        alert('Set password failed: ' + JSON.stringify(error));
      })
    },


    config_wifi: function () {
      this.wifiConfirm = false;
      this.rebooting = true;

      var config = {
        mode: this.wifi_mode,
        channel: this.wifi_ch,
        ssid: this.wifi_ssid,
        pass: this.wifi_pass
      }

      api.put('wifi', config).fail(function (error) {
        alert('Failed to configure WiFi: ' + JSON.stringify(error));
      })
    }
  }
}
