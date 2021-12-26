/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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
  template: '#settings-admin-template',
  props: ['config', 'state'],


  data: function () {
    return {
      enableKeyboard: true,
      autoCheckUpgrade: true,
      password: '',
      firmwareName: '',
      show: {
        upgrade: false,
        upgrading: false,
        upload: false
      }
    }
  },


  ready: function () {
    this.enableKeyboard = this.config.admin['virtual-keyboard-enabled']
    this.autoCheckUpgrade = this.config.admin['auto-check-upgrade']
  },


  methods: {

    shutdown: function() {
      api.put('shutdown');
    },
    reboot: function() {
      api.put('reboot');
    },

    backup: function () {
      document.getElementById('download-target').src = '/api/config/download';
    },


    restore_config: function () {
      this.$root.upload({
        accept: '.json',
        on_confirm: this.restore
      })
    },


    restore: function (file, done) {
      var fr = new FileReader();
      fr.onload = function (e) {
        var config;

        try {
          config = JSON.parse(e.target.result);
        } catch (ex) {
          this.$root.error_dialog("Invalid config file");
          return done(false);
        }

        api.put('config/save', config).done(function (data) {
          this.$dispatch('update');
          this.$root.success_dialog('Configuration restored.');

        }.bind(this)).fail(function (error) {
          this.$root.api_error('Restore failed', error);

        }.bind(this)).always(function () {done(false)})
      }.bind(this);

      fr.readAsText(file);
    },


    do_reset: function () {
      api.put('config/reset').done(function () {
        this.$dispatch('update');
        this.$root.success_dialog('Configuration reset.')

      }.bind(this)).fail(function (error) {
        this.$root.api_error('Reset failed', error);
      }.bind(this));
    },


    reset: function () {
      this.$root.open_dialog({
        title: 'Reset to default configuration?',
        body: 'Non-network configuration changes will be lost.',
        buttons: 'Cancel OK',
        callback: {ok: this.do_reset}
      })
    },

    check: function () {this.$dispatch('check', true)},


    upgrade: function () {
      this.password = '';
      this.show.upgrade = true;
    },


    upgrade_confirmed: function () {
      this.show.upgrade = false;

      api.put('upgrade', {password: this.password}).done(function () {
        this.show.upgrading = true;

      }.bind(this)).fail(function () {
        this.error_dialog('Invalid password');
      }.bind(this))
    },


    upload: function () {
      this.password = '';

      this.$root.upload({
        url: 'firmware/update',
        accept: '.bz2',
        form: function(file) {
          return {
            firmware: file,
            password: this.password
          }
        }.bind(this),

        on_confirm: function (file, ok) {
          this.confirm_cb = ok;
          this.show.upload = true;
          this.firmwareName = file.name;
        }.bind(this),

        on_success: function (file, next) {
          this.show.upgrading = true;
          next();
        }.bind(this),

        on_failure: function (file, error) {
          this.error_dialog('Invalid password or bad firmware');
        }.bind(this)
      })
    },


    upload_confirmed: function () {this.confirm_cb(true)},


    change_auto_check_upgrade: function () {
      this.config.admin['auto-check-upgrade'] = this.autoCheckUpgrade
      this.$dispatch('config-changed')
    },


    change_enable_keyboard: function () {
      this.config.admin['virtual-keyboard-enabled'] = this.enableKeyboard
      this.$dispatch('config-changed')
      if (!this.enableKeyboard) api.put('keyboard/hide')
    }
  }
}
