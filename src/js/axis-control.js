'use strict'


module.exports = {
  template: '#axis-control-template',
  props: ['axes', 'colors', 'enabled'],


  methods: {
    jog: function (axis, move) {
      this.$dispatch('jog', this.axes[axis], move)
    }
  }
}
