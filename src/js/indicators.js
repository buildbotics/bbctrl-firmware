'use strict'


module.exports = {
  template: '#indicators-template',
  props: ['state'],


  methods: {
    is_motor_enabled: function (motor) {
      return typeof this.state[motor + 'pm'] != 'undefined' &&
        this.state[motor + 'pm'];
    },


    get_class: function (code) {
      if (typeof this.state[code] != 'undefined') {
        var state = this.state[code];

        if (state == 0 || state === false) return 'logic-lo fa-circle';
        if (state == 1 || state === true) return 'logic-hi fa-circle';
        if (state == 2) return 'logic-tri fa-circle-o';
      }

      return 'fa-exclamation-triangle warn';
    }
  }
}
