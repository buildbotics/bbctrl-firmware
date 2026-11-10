/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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


module.exports = {
  template: '#overrides-template',
  props: ['state', 'template'],

  data() {
    return {
      speed: 1,
      feed: 1
    }
  },


  // Follow the controller, which may report after this mounts or reset the
  // override itself
  watch: {
    'state.so'(value) {if (value) this.speed = value},
    'state.fo'(value) {if (value) this.feed  = value}
  },


  ready() {
    this.speed = this.state.so || 1
    this.feed  = this.state.fo || 1
  },


  methods: {
    // Two decimals hide float noise from the 0.1 steps, such as 0.30000004
    async override_speed() {
      return this.$api.put('override/speed/' + this.speed.toFixed(2))
    },


    async reset_speed() {
      this.speed = 1
      return this.override_speed()
    }
  }
}
