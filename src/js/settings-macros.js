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

var util   = require('./util');


module.exports = {
  template: '#settings-macros-template',
  props: ['config', 'state', 'template'],


  data: function () {
    return {
      dragging: -1
    }
  },


  computed: {
    macros: function () {return this.config.macros || []}
  },


  methods: {
    add: function () {
      this.macros.push({name: '', path: '', color: '#e6e6e6'})
    },


    mousedown: function (event) {this.target = event.target},


    dragstart: function(event) {
      if (this.target.localName == 'input') event.preventDefault();
    },


    drag: function(index) {
      console.log('drag(' + index + ')');
      this.dragging = index;
      event.preventDefault();
    },


    drop: function(index) {
      console.log('drop(' + index + ') dragging=' + this.dragging);

      if (index == this.dragging) return;
      var item = this.macros[this.dragging];
      this.macros.splice(this.dragging, 1);
      this.macros.splice(index, 0, item);
      this.change();
    },


    remove: function(index) {
      this.macros.splice(index, 1);
      this.change();
    },


    open: function(index) {
      this.$root.file_dialog({
        callback: function (path) {
          if (path) {
            this.macros[index].path = util.display_path(path);
            this.change();
          }
        }.bind(this)
      })
    },


    change: function () {this.$dispatch('input-changed')}
  }
}
