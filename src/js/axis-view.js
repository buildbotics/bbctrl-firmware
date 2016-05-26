'use strict'


module.exports = {
  template: '#axis-view-template',
  props: ['index', 'config', 'template'],


  data: function () {
    return {
      active: false,
      axis: {}
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


  attached: function () {this.active = true; this.update()},
  detached: function () {this.active = false},


  methods: {
    update: function () {
      if (!this.active) return;

      Vue.nextTick(function () {
        if (this.config.hasOwnProperty('axes'))
          this.axis = this.config.axes[this.index];
        else this.axes = {};

        var template = this.template.axes;
        for (var category in template)
          for (var key in template[category])
            if (!this.axis.hasOwnProperty(key))
              this.$set('axis["' + key + '"]',
                        template[category][key].default);
      }.bind(this));
    }
  }
}
