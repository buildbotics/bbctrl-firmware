'use strict'


module.exports = {
  template: '#switches-view-template',
  props: ['config', 'template'],


  data: function () {
    return {
      'switches': []
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
        if (this.config.hasOwnProperty('switches'))
          this.switches = this.config.switches;
        else this.switches = [];

        for (var i = 0; i < this.switches.length; i++) {
          var template = this.template.switches;
          for (var key in template)
            if (!this.switches[i].hasOwnProperty(key))
              this.$set('switches[' + i + ']["' + key + '"]',
                        template[key].default);
        }
      }.bind(this));
    }
  }
}
