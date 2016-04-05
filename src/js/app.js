'use strict'

var api = require('./api');


module.exports = new Vue({
  el: 'body',


  data: function () {
    return {
      currentView: 'loading',
      index: -1,
      modified: false,
      template: {"motors": {}, "axes": {}},
      config: {"motors": [{}]}
    }
  },


  components: {
    'loading-view': {template: '<h1>Loading...</h1>'},
    'control-view': require('./control-view'),
    'axis-view': require('./axis-view'),
    'motor-view': require('./motor-view'),
    'spindle-view': require('./spindle-view'),
    'switches-view': require('./switches-view'),
    'gcode-view': require('./gcode-view'),
    'admin-view': require('./admin-view')
  },


  events: {
    'config-changed': function () {this.modified = true;}
  },


  ready: function () {
    $.get('/config-template.json').success(function (data, status, xhr) {
      this.template = data;

      api.get('load').done(function (data) {
        this.config = data;

        this.parse_hash();
        $(window).on('hashchange', this.parse_hash);
     }.bind(this))
    }.bind(this))
  },


  methods: {
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
      api.post('save', this.config).done(function (data) {
        this.modified = false;
      }.bind(this)).fail(function (xhr, status) {
        alert('Save failed: ' + status + ': ' + xhr.responseText);
      });
    }
  }
})
