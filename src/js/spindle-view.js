'use strict'


module.exports = {
  template: '#spindle-view-template',
  props: ['config', 'template'],


  data: function () {
    return {
      spindle: {}
    }
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
        if (this.config.hasOwnProperty('spindle'))
          this.spindle = this.config.spindle;

        var template = this.template.spindle;
        for (var key in template)
          if (!this.spindle.hasOwnProperty(key))
            this.$set('spindle["' + key + '"]',
                      template[key].default);
      }.bind(this));
    }
  }
}
