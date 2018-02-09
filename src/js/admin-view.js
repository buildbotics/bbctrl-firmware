'use strict'


var api = require('./api');


module.exports = {
  template: '#admin-view-template',
  props: ['config', 'state'],


  data: function () {
    return {
      configRestored: false,
      confirmReset: false,
      configReset: false,
      hostnameSet: false,
      usernameSet: false,
      passwordSet: false,
      redirectTimeout: 0,
      latest: '',
      hostname: '',
      username: '',
      current: '',
      password: '',
      password2: '',
      autoCheckUpgrade: true,
    }
  },


  events: {
    connected: function () {
      if (this.firmwareUpgrading) location.reload(true);
    },

    latest_version: function (version) {this.latest = version}
  },


  ready: function () {
    this.autoCheckUpgrade = this.config.admin['auto-check-upgrade']

    api.get('hostname').done(function (hostname) {
      this.hostname = hostname;
    }.bind(this));

    api.get('remote/username').done(function (username) {
      this.username = username;
    }.bind(this));
  },


  methods: {
    redirect: function (hostname) {
      if (0 < this.redirectTimeout) {
        this.redirectTimeout -= 1;
        setTimeout(function () {this.redirect(hostname)}.bind(this), 1000);

      } else {
        location.hostname = hostname;
        this.hostnameSet = false;
      }
    },


    set_hostname: function () {
      api.put('hostname', {hostname: this.hostname}).done(function () {
        this.redirectTimeout = 45;
        this.hostnameSet = true;

        api.put('reboot').always(function () {
          var hostname = this.hostname;
          if (String(location.hostname).endsWith('.local'))
            hostname += '.local';
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


    change_auto_check_upgrade: function () {
      this.config.admin['auto-check-upgrade'] = this.autoCheckUpgrade;
      this.$dispatch('config-changed');
    }
  }
}
