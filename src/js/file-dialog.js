/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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


var util = require('./util');


module.exports = {
  template: '#file-dialog-template',
  props: ['locations'],


  data: function () {
    return {
      show: false,
      config: {},
      selected: undefined,
      dir: false
    }
  },


  methods: {
    open: function (config) {
      this.config = config;
      this.show = true;
      this.$refs.files.open(config.dir || '/');
    },


    set_selected: function (path, dir) {
      this.selected = path;
      this.dir = dir;
    },


    respond: function (path) {
      if (this.config.callback) this.config.callback(path);
    },


    response: function (path) {
      this.show = false;

      if (path && this.config.save) {
        var filename = util.basename(path);
        var exists = this.$refs.files.has_file(filename);

        if (exists) {
          this.$root.open_dialog({
            title: 'Overwrite file?',
            body: 'Overwrite <tt>' + filename + '</tt>?',
            buttons: 'No Yes',
            callback: {
              no: this.respond,
              yes: function () {this.respond(path)}.bind(this)
            }
          })

          return;
        }
      }

      this.respond(path);
    },


    ok: function () {
      if (this.dir) this.$refs.files.open(this.selected);
      else this.response(this.selected)
    },


    cancel: function () {this.response()}
  }
}
