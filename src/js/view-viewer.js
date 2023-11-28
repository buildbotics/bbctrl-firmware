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
  template: '#view-viewer-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      loading: false,
      retry: 0,
      path: undefined,
      toolpath: {},
      program: {},
      snaps: 'angled top bottom front back left right'.split(' ')
    }
  },


  components: {
    'viewer-help-dialog': require('./viewer-help-dialog'),
    'path-viewer':        require('./path-viewer')
  },


  computed: {
    filename() {return util.display_path(this.path)},


    show() {
      if (this.$refs.viewer == undefined) return {}
      return this.$refs.viewer.show
    }
  },


  attached() {this.load(this.$root.selected_program.path)},


  methods: {
    async load(path) {
      if (this.path == path)
        return Vue.nextTick(this.$refs.viewer.update_view)

      this.path = path
      this.toolpath = {}
      this.$refs.viewer.clear()

      if (!path) return
      this.loading = true
      this.program = this.$root.select_path(path)

      try {
        let toolpath = await this.program.view()
        if (path != this.path) return
        this.toolpath = toolpath
        Vue.nextTick(this.$refs.viewer.update)

      } finally {
        if (path == this.path) this.loading = false
      }
    },


    async open() {
      let path = await this.$root.file_dialog({dir: util.dirname(this.path)})
      if (path) this.load(path)
    },


    toggle(name) {this.$refs.viewer.toggle(name)},
    snap(view) {this.$refs.viewer.snap(view)}
  }
}
