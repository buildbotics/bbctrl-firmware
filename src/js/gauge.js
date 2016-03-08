module.exports = Vue.extend({
  template: '<div class="gauge"><canvas></canvas><span></span></div>',


  props: {
    value: {tupe: Number, required: true},
    lines: {type: Number, default: 1},
    angle: {type: Number, default: 0.1},
    lineWidth: {type: Number, default: 0.25},
    ptrLength: {type: Number, default: 0.85},
    ptrStrokeWidth: {type: Number, default: 0.054},
    ptrColor: {type: String, default: '#666'},
    limitMax: {type: Boolean, default: true},
    colorStart: {type: String, default: '#6FADCF'},
    colorStop: {type: String, default: '#8FC0DA'},
    strokeColor: String,
    min: {type: Number, default: 0},
    max: {type: Number, default: 100}
  },


  data: function () {
    return {
    }
  },


  watch: {
    value: function (value) {this.set(value)}
  },


  compiled: function () {
    this.gauge = new Gauge(this.$el.children[0]).setOptions({
      lines: this.lines,
      angle: this.angle,
      lineWidth: this.lineWidth,
      pointer: {
        length: this.ptrLength,
        strokeWidth: this.ptrStrokeWidth,
        color: this.ptrColor
      },
      limitMax: this.limitMax,
      colorStart: this.colorStart,
      colorStop: this.colorStop,
      strokeColor: this.strokeColor,
      minValue: this.min,
      maxValue: this.max
   });

    this.gauge.minValue = parseInt(this.min);
    this.gauge.maxValue = parseInt(this.max);
    this.set(this.value);

    this.gauge.setTextField(this.$el.children[1]);
  },


  methods: {
    set: function (value) {
      if (typeof value == 'number') this.gauge.set(value);
    }
  }
})
