'use strict'


var api = require('./api');


module.exports = {
  template: '#admin-view-template',
  props: ['config'],


  data: function () {
    return {
      configRestored: false,
      confirmReset: false,
      configReset: false,
      firmwareUpgrading: false,
      latest: '',
    }
  },


  events: {
    connected: function () {
      if (this.firmwareUpgrading) location.reload(true);
    }
  },


  ready: function () {},


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
        } catch (e) {
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


    check: function () {
      $.ajax({
        type: 'GET',
        url: 'https://buildbotics.com/bbctrl/latest.txt',
        cache: false

      }).done(function (data) {
        this.latest = data;

      }.bind(this)).fail(function (error) {
        alert('Failed to get latest version information');
      });
    },


    upgrade: function () {
      this.firmwareUpgrading = true;
      api.put('upgrade');
    }
  }
}
