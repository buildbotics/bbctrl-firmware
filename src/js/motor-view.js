'use strict'


module.exports = {
  template: '#motor-view-template',
  props: ['index', 'config', 'template'],


  data: function () {
    return {
      active: false,
      motor: {}
    }
  },


  watch: {
    index: function() {this.update()}
  },


  events: {
    'input-changed': function() {
      this.slave_update();
      this.$dispatch('config-changed');
      return false;
    }
  },


  attached: function () {this.active = true; this.update()},
  detached: function () {this.active = false},


  methods: {
    slave_update: function () {
      var slave = false;
      for (var i = 0; i < this.index; i++)
        if (this.motor.axis == this.config.motors[i].axis)
          slave = true;

      var el = $(this.$el);

      if (slave) {
        el.find('.axis .units').text('(slave motor)');
        el.find('.limits, .homing, .motion').find('input, select')
          .attr('disabled', 1);
        el.find('.power-mode select').attr('disabled', 1);
        el.find('.motion .reverse input').removeAttr('disabled');

      } else {
        el.find('.axis .units').text('');
        el.find('input,select').removeAttr('disabled');
      }
    },


    update: function () {
      if (!this.active) return;

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

        this.slave_update();
      }.bind(this));
    }
  }
}
