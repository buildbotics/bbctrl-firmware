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
  template: '#view-files-template',
  props: ['state'],


  data() {
    return {
      first:    true,
      selected: '',
      is_dir:   false
    }
  },


  attached() {
    if (this.first) this.first = false
    else this.$refs.files.reload()
  },


  methods: {
    upload() {this.$refs.files.upload()},
    new_folder() {this.$refs.files.new_folder()},


    set_selected(path, is_dir) {
      this.selected = path
      this.is_dir   = is_dir
    },


    edit() {
      if (this.selected && !this.is_dir) this.$root.edit(this.selected)
    },


    view() {
      if (this.selected && !this.is_dir) this.$root.view(this.selected)
    },


    download() {
      if (this.selected && !this.is_dir) this.$els.download.click()
    },


    async delete() {
      if (!this.selected) return

      let filename = util.basename(this.selected)

      let result = await this.$root.open_dialog({
        header: 'Delete ' + (this.is_dir ? 'directory' : 'file') + '?',
        body: 'Are you sure you want to delete <tt>' + filename +
          (this.is_dir ? '</tt> and all the files under it?' : '</tt>?'),
        buttons: 'Cancel Ok'
      })

      if (result == 'ok') {
        await this.$api.delete('fs/' + this.selected)
        await this.$refs.files.reload()
      }
    }
  }
}
