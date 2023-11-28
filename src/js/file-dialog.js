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
  template: '#file-dialog-template',
  props: ['locations'],


  data() {
    return {
      show:     false,
      config:   {},
      selected: undefined,
      is_dir:   false,
    }
  },


  computed: {
    buttons() {
      return [
        'Cancel',
        {text: this.config.save ? 'Save' : 'Open', disabled: !this.selected}
      ]
    }
  },


  methods: {
    set_selected(path, is_dir) {
      this.selected = path
      this.is_dir   = is_dir
    },


    activate(path) {
      this.set_selected(path)
      this.$refs.dialog.close('activate')
    },


    async open(config = {}) {
      this.config = config

      this.$refs.files.open(config.dir || '/')
      let response = await this.$refs.dialog.open()

      if (response == 'cancel') return

      // Open directory
      if (response == 'open' && this.is_dir) {
        config = Object.assign({}, config, {dir: this.selected})
        return this.open(config)
      }

      let path = this.selected

      // Confirm overwrite
      if (path && this.config.save) {
        let filename = util.basename(path)
        let exists = this.$refs.files.has_file(filename)

        if (exists) {
          let response = await this.$root.open_dialog({
            header:  'Overwrite file?',
            body:    'Overwrite <tt>' + filename + '</tt>?',
            buttons: 'No Yes'
          })

          return response == 'yes' ? path : undefined
        }
      }

      return path
    }
  }
}
