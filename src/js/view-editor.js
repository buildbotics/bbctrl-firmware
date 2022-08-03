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

var api  = require('./api')
var util = require('./util')


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
      this.check_save(() => {location.hash = path.join(':')})
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


    _load(path, ok) {
      this.path = path || ''
      if (!path) return

      this.loading = true

      this.$root.select_path(path).load()
        .done((data) => {
          if (this.path == path) {
            this.set(data)
            if (ok) ok()
          }

        }).always(() => {
          if (this.path == path) this.loading = false

        }).fail(this.$root.api_error)
    },


    load(path) {
      if (this.path == path) return
      this.check_save(() => {this._load(path)})
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


    check_save(ok) {
      if (!this.modified) return ok()

      this.$root.open_dialog({
        title: 'Save file?',
        body: 'The current file has been modified.  ' +
          'Would you like to save it first?',
        width: '320px',
        buttons: [
          {
            text: 'Cancel',
            title: 'Stay on the editor page.'
          }, {
            text: 'Discard',
            title: "Discard changes."
          }, {
            text: 'Save',
            title: 'Save changes.',
            'class': 'button-success'
          }
        ],
        callback: {
          save() {this.save(ok)},
          discard() {this.revert(ok)}
        }
      })
    },


    new_file() {
      this.check_save(() => {
        this.path = ''
        this.set('')
      })
    },


    open() {
      this.check_save(() => {
        this.$root.file_dialog({
          callback: (path) => {if (path) this.load(path)},
          dir: this.path ? util.dirname(this.path) : '/'
        })
      })
    },


    do_save(path, ok) {
      var fd = new FormData()
      var file = new File([new Blob([this.doc.getValue()])], path)
      fd.append('file', file)

      api.upload('fs/' + path, fd)
        .done(() => {
          this.path = path
          this.modified = false
          this.doc.markClean()
          if (typeof ok != 'undefined') ok()

        }).fail((error) => {
          this.$root.error_dialog({body: 'Save failed'})
        })
    },


    save(ok) {
      if (!this.path) this.save_as(ok)
      else this.do_save(this.path, ok)
    },


    save_as(ok) {
      this.$root.file_dialog({
        save: true,
        callback: (path) => {if (path) this.do_save(path, ok)},
        dir: this.path ? util.dirname(this.path) : '/'
      })
    },


    revert(ok) {
      if (this.modified) {
        this.modified = false
        this._load(this.path, ok)
      }
    },


    download() {
      var data = new Blob([this.doc.getValue()], {type: 'text/plain'})
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
