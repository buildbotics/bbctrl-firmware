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
  template: '#admin-general-view-template',
  props: ['config', 'state'],


  data: function () {
    return {
      configRestored: false,
      confirmReset: false,
      configReset: false,
      latest: '',
      autoCheckUpgrade: true
    }
  },


  events: {
    latest_version: function (version) {this.latest = version}
  },


  ready: function () {
    this.autoCheckUpgrade = this.config.admin['auto-check-upgrade']
  },


  methods: {
    backup: function () {
      document.getElementById('download-target').src = '/api/config/download';
    },


    restore: function (e) {
      var files = e.target.files || e.dataTransfer.files;
      if (!files.length) return;

      var fr = new FileReader();
      fr.onload = function (e) {
        var config;

        try {
          config = JSON.parse(e.target.result);
        } catch (ex) {
          alert("Invalid config file");
          return;
        }

        api.put('config/save', config).done(function (data) {
          this.$dispatch('update');
          this.configRestored = true;

        }.bind(this)).fail(function (error) {
          alert('Restore failed: ' + error);
        })
      }.bind(this);

      fr.readAsText(files[0]);
    },


    reset: function () {
      this.confirmReset = false;
      api.put('config/reset').done(function () {
        this.$dispatch('update');
        this.configReset = true;

      }.bind(this)).fail(function (error) {
        alert('Reset failed: ' + error);
      });
    },


    check: function () {this.$dispatch('check')},
    upgrade: function () {this.$dispatch('upgrade')},


    upload: function (e) {
      var files = e.target.files || e.dataTransfer.files;
      if (!files.length) return;
      this.$dispatch('upload', files[0]);
    },


    change_auto_check_upgrade: function () {
      this.config.admin['auto-check-upgrade'] = this.autoCheckUpgrade;
      this.$dispatch('config-changed');
    }
  }
}
