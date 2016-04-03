'use strict'


module.exports = {
  template: '#axis-control-template',
  props: ['axes', 'colors'],


  methods: {
    jog: function (axis, move) {
      this.$dispatch('jog', this.axes[axis], move)
    },


    home: function (axis) {
      this.$dispatch('home', this.axes[axis])
    },


    zero: function (axis) {
      this.$dispatch('zero', this.axes[axis])
    }
  }
}
