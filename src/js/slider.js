module.exports = new Vue({
  template: '<input type="range" v-model="value">',
  replace: true,


  props: {
    value: Number,
    min: Number,
    max: Number,
    step: Number
  },


  data: function () {
    return {
    }
  },


  compiled: function () {
    this.$el.attributes.min = this.min;
    this.$el.attributes.max = this.max;
    this.$el.attributes.step = this.step;
  }
}
