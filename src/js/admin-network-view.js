/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

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
      wifi_internal: true,
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
      this.wifi_mode     = config.mode;
      this.wifi_internal = config.internal;
      this.wifi_ssid     = config.ssid;
      this.wifi_ch       = config.channel;
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
          if (String(location.hostname) == 'localhost') return;

          var hostname = this.hostname;
          if (String(location.hostname).endsWith('.local'))
            hostname += '.local'
          this.$dispatch('hostname-changed', hostname);
          this.redirect(hostname);
        }.bind(this));

      }.bind(this)).fail(function (error) {
        api.alert('Set hostname failed', error);
      })
    },


    set_username: function () {
      api.put('remote/username', {username: this.username}).done(function () {
        this.usernameSet = true;
      }.bind(this)).fail(function (error) {
        api.alert('Set username failed', error);
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
        api.alert('Set password failed', error);
      })
    },


    config_wifi: function () {
      this.wifiConfirm = false;

      if (!this.wifi_ssid.length) {
        alert('SSID not set');
        return;
      }

      if (32 < this.wifi_ssid.length) {
        alert('SSID longer than 32 characters');
        return;
      }

      if (this.wifi_pass.length && this.wifi_pass.length < 8) {
        alert('WiFi password shorter than 8 characters');
        return;
      }

      if (128 < this.wifi_pass.length) {
        alert('WiFi password longer than 128 characters');
        return;
      }

      this.rebooting = true;

      var config = {
        mode:     this.wifi_mode,
        internal: this.wifi_internal,
        channel:  this.wifi_ch,
        ssid:     this.wifi_ssid,
        pass:     this.wifi_pass
      }

      api.put('wifi', config).fail(function (error) {
        api.alert('Failed to configure WiFi', error);
        this.rebooting = false;
      }.bind(this))
    }
  }
}
