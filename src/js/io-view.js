'use strict'


module.exports = {
  template: '#io-view-template',
  props: ['config', 'template'],


  data: function () {
    return {
      switches: {},
      outputs: {}
    }
  },


  events: {
    'input-changed': function() {
      this.$dispatch('config-changed');
      return false;
    }
  },


  ready: function () {this.update()},


  methods: {
    update: function () {
      Vue.nextTick(function () {
        // Switches
        if (this.config.hasOwnProperty('switches'))
          this.switches = this.config.switches;
        else this.switches = {};

        var template = this.template.switches;
        for (var key in template)
          if (!this.switches.hasOwnProperty(key))
            this.$set('switches["' + key + '"]', template[key].default);

        // Outputs
        if (this.config.hasOwnProperty('outputs'))
          this.outputs = this.config.outputs;
        else this.outputs = {};

        var template = this.template.outputs;
        for (var key in template)
          if (!this.outputs.hasOwnProperty(key))
            this.$set('outputs["' + key + '"]', template[key].default);
      }.bind(this));
    }
  }
}
