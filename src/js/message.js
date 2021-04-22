/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

'use strict'


module.exports = {
  template: '#message-template',

  props: {
    show: {
      type: Boolean,
      required: true,
      twoWay: true
    },

    click_away_close: {
      type: Boolean,
      default: true
    },

    width: {
      type: String,
      default: ''
    }
  },


  events: {
    'click-away': function () {if (this.click_away_close) this.show = false}
  },


  watch: {
    show: function (show) {if (show) Vue.nextTick(this.focus)}
  },


  methods: {
    click_away: function (e) {
      if (!e.target.classList.contains('modal-wrapper')) return;
      this.$emit('click-away')
    },


    focus: function () {
      $(this.$el).find('[focus]').each(function (index, e) {
        e.focus()
        return false;
      })
    }
  }
}
