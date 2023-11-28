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
  template: '#video-template',

  attached() {Vue.nextTick(this.resize)},


  ready() {
    window.addEventListener('resize', this.resize, false)
  },


  methods: {
    reload() {this.$els.img.src = '/api/video?' + Math.random()},


    resize() {
      let width = this.$els.video.clientWidth
      let height = this.$els.video.clientHeight
      let aspect = 3 / 4 // TODO should probably not be hard coded

      if (!width) return

      width = Math.min(width, height / aspect)
      height = Math.min(height, width * aspect)

      this.$els.img.style.width  = width  + 'px'
      this.$els.img.style.height = height + 'px'
    }
  }
}
