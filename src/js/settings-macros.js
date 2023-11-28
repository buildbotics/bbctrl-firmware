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


let util = require('./util')


module.exports = {
  template: '#settings-macros-template',
  props: ['config', 'state', 'template'],


  data() {
    return {
      dragging: -1
    }
  },


  computed: {
    macros() {return this.config.macros || []}
  },


  methods: {
    add() {
      this.macros.push({name: '', path: '', color: '#e6e6e6'})
    },


    mousedown(event) {this.target = event.target},


    dragstart(event) {
      if (this.target.localName == 'input') event.preventDefault()
    },


    drag(index) {
      console.log('drag(' + index + ')')
      this.dragging = index
      event.preventDefault()
    },


    drop(index) {
      console.log('drop(' + index + ') dragging=' + this.dragging)

      if (index == this.dragging) return
      let item = this.macros[this.dragging]
      this.macros.splice(this.dragging, 1)
      this.macros.splice(index, 0, item)
      this.change()
    },


    remove(index) {
      this.macros.splice(index, 1)
      this.change()
    },


    async open(index) {
      let path = await this.$root.file_dialog()
      if (path) {
        this.macros[index].path = util.display_path(path)
        this.change()
      }
    },


    change() {this.$dispatch('input-changed')}
  }
}
