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



module.exports = {
  template: '#color-picker-template',
  props: {
    value: {
      type: String,
      required: true,
      twoWay: true
    }
  },


  data() {
    return {
      config: '{}'
    }
  },


  ready() {
    window.jscolor.install(this.$el)
    this.jscolor = this.$els.input.jscolor
    this.jscolor.option({
      onChange: this.change,
      onInput: this.change,
      showOnClick: false,
      hideOnPaletteClick: true,
      zIndex: 500,
      palette: [
        '#e6e6e6', '#7d7d7d', '#870014', '#ec1c23', '#ff7e26',
        '#fef100', '#22b14b', '#00a1e7', '#3f47cc', '#a349a4',
        '#ffffff', '#c3c3c3', '#b87957', '#feaec9', '#ffc80d',
        '#eee3af', '#b5e61d', '#99d9ea', '#7092be', '#c8bfe7'
      ]})
  },


  methods: {
    change() {
      this.value = this.jscolor.toHEXString()
      this.$emit('change', this.value)
    },


    show() {this.jscolor.show()}
  }
}
