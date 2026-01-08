/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2026, Buildbotics LLC, All rights reserved.

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
  template: '#view-editor-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      loading: false,
      path: undefined,
      modified: false,
      canRedo: false,
      canUndo: false,
      clipboard: ''
    }
  },


  computed: {
    filename() {
      return (this.modified ? '* ' : '') +
        (util.display_path(this.path) || '(unnamed)')
    },


    basename() {
      return this.path ? util.basename(this.path) : 'unnamed.txt'
    }
  },


  events: {
    'route-changing'(path, cancel) {
      if (!this.modified || path[0] == 'editor') return
      cancel()
      this.check_save(ok => {if (ok) location.hash = path.join(':')})
    }
  },


  attached() {
    if (!this.editor) {
      this.editor = CodeMirror.fromTextArea(this.$els.textarea, {
        lineNumbers: true,
        mode: 'gcode'
      })
      this.doc = this.editor.getDoc()
      this.doc.on('change', this.change)
    }

    this.load(this.$root.selected_program.path)
    Vue.nextTick(() => {this.editor.refresh()})
  },


  methods: {
    change() {
      this.modified = !this.doc.isClean()

      let size = this.doc.historySize()
      this.canRedo = !!size.redo
      this.canUndo = !!size.undo
    },


    async _load(path) {
      this.path = path || ''
      if (!path) return true

      this.loading = true

      try {
        let data = await (this.$root.select_path(path).load())

        if (this.path == path) {
          this.set(data)
          return true
        }

      } finally {
        if (this.path == path) this.loading = false
      }
    },


    async load(path) {
      if (this.path == path) return true
      if (await this.check_save()) return this._load(path)
    },


    set(text) {
      this.doc.setValue(text)
      this.doc.clearHistory()
      this.doc.markClean()
      this.modified = false
      this.canRedo = false
      this.canUndo = false
      if (this.path)
        this.editor.setOption('mode', util.get_highlight_mode(this.path))
    },


    async check_save() {
      if (!this.modified) return true

      let response = await this.$root.open_dialog({
        header: 'Save file?',
        body:   'The current file has been modified.  ' +
                'Would you like to save it first?',
        width:  '320px',
        buttons: [
          {
            text:  'Cancel',
            title: 'Stay on the editor page.'
          }, {
            text:  'Discard',
            title: 'Discard changes.'
          }, {
            text:  'Save',
            title: 'Save changes.',
            class: 'button-success'
          }
        ]
      })

      if (response == 'save')    return this.save()

      if (response == 'discard') {
        this.modified = false
        this.doc.markClean()
        return true
      }

      return false // Cancel - stay in editor
    },


    async new_file() {
      if (await this.check_save()) {
        this.path = ''
        this.set('')
      }
    },


    async open() {
      if (await this.check_save()) {
        let path = await this.$root.file_dialog({
          dir: this.path ? util.dirname(this.path) : '/'
        })

        if (path) return this.load(path)
      }
    },


    async do_save(path) {
      try {
        let fd   = new FormData()
        let file = new File([new Blob([this.doc.getValue()])], path)
        fd.append('file', file)

        await this.$api.put('fs/' + path, fd)

        this.path     = path
        this.modified = false
        this.doc.markClean()

        return true // Success

      } catch (e) {
        await this.$root.error_dialog('Failed to save file: ' + e.message)
        return false
      }
    },


    async save() {
      if (this.path) return this.do_save(this.path)
      return this.save_as()
    },


    async save_as() {
      let path = await this.$root.file_dialog({
        save: true,
        dir: this.path ? util.dirname(this.path) : '/'
      })

      if (path) return this.do_save(path)
      return false
    },


    async revert() {
      if (!this.modified) return true // clean

      this.modified = false
      return this._load(this.path)
    },


    download() {
      let data = new Blob([this.doc.getValue()], {type: 'text/plain'})
      window.URL.revokeObjectURL(this.$els.download.href)
      this.$els.download.href = window.URL.createObjectURL(data)
      this.$els.download.click()
    },


    view() {this.$root.view(this.path)},
    undo() {this.doc.undo()},
    redo() {this.doc.redo()},


    cut() {
      this.clipboard = this.doc.getSelection()
      this.doc.replaceSelection('')
    },


    copy()  {this.clipboard = this.doc.getSelection()},
    paste() {this.doc.replaceSelection(this.clipboard)}
  }
}
