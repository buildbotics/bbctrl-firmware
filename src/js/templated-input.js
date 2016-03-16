'use strict'


module.exports = {
  replace: true,
  template: '#templated-input-template',
  props: ['name', 'model', 'template'],


  methods: {
    change: function () {
      this.$dispatch('input-changed');
    }
  }
}
