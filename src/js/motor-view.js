'use strict'


module.exports = {
  template: '#motor-view-template',
  props: ['index', 'config', 'template'],


  data: function () {
    return {
      motor: {}
    }
  },


  watch: {
    index: function() {this.update();}
  },


  events: {
    'input-changed': function() {
      this.$dispatch('config-changed');
      return false;
    }
  },


  ready: function () {
    this.update();
  },


  methods: {
    update: function () {
      Vue.nextTick(function () {
        if (this.config.hasOwnProperty('motors'))
          this.motor = this.config.motors[this.index];
        else this.motor = {};

        var template = this.template.motors;
        for (var category in template)
          for (var key in template[category])
            if (!this.motor.hasOwnProperty(key))
              this.$set('motor["' + key + '"]',
                        template[category][key].default);
      }.bind(this));
    }
  }
}
