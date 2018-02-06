'use strict'


module.exports = {
  template: '#axis-control-template',
  props: ['axes', 'colors', 'enabled'],


  methods: {
    jog: function (axis, power) {this.$dispatch('jog', this.axes[axis], power)},
    release: function (axis) {this.$dispatch('jog', this.axes[axis], 0)}
  }
}
