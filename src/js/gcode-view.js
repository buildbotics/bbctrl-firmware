'use strict'


module.exports = {
  template: '#gcode-view-template',
  props: ['config', 'template'],


  data: function () {
    return {
      gcode: {}
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
        if (this.config.hasOwnProperty('gcode'))
          this.gcode = this.config.gcode;

        var template = this.template.gcode;
        for (var key in template)
          if (!this.gcode.hasOwnProperty(key))
            this.$set('gcode["' + key + '"]', template[key].default);
      }.bind(this));
    }
  }
}
