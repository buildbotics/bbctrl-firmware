'use strict'


module.exports = {
  template: '#config-view-template',


  data: function () {
    return {
      page: 'motor',
      motor: 0,
      template: {},
      config: {"motors": [{}]}
    }
  },


  components: {
    'motor-view': require('./motor-view'),
    'switch-view': require('./switch-view')
  },


  ready: function () {
    $.get('/config-template.json').success(function (data, status, xhr) {
      this.template = data;

      $.get('/default-config.json').success(function (data, status, xhr) {
        this.config = data;
      }.bind(this))
    }.bind(this))
  },


  methods: {
    back: function() {
      if (this.motor) this.motor--;
    },

    next: function () {
      if (this.motor < this.config.motors.length - 1) this.motor++;
    }
  }
}
