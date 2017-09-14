'use strict'


module.exports = {
  template: '#tool-view-template',
  props: ['config', 'template'],


  data: function () {
    return {
      tool: {}
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
        if (this.config.hasOwnProperty('tool'))
          this.tool = this.config.tool;

        var template = this.template.tool;
        for (var key in template)
          if (!this.tool.hasOwnProperty(key))
            this.$set('tool["' + key + '"]', template[key].default);
      }.bind(this));
    }
  }
}
