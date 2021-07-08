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

'use strict';

var api    = require('./api');
var util   = require('./util');
var cookie = require('./cookie');


module.exports = {
  template: '#view-editor-template',
  props: ['config', 'template', 'state'],


  data: function () {
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
    filename: function () {
      return (this.modified ? '* ' : '') +
        (util.display_path(this.path) || '(unnamed)')
    },


    basename: function () {
      return this.path ? util.basename(this.path) : 'unnamed.txt';
    }
  },


  watch: {
    'state.queued': function () {
      if (!this.path && this.state.queued) this.load(this.state.queued);
    }
  },


  events: {
    'route-changing': function (path, cancel) {
      if (!this.modified || path[0] == 'editor') return;
      cancel();
      this.check_save(function () {location.hash = path.join(':')});
    }
  },


  attached: function () {
    if (typeof this.doc == 'undefined') return;
    this.load(cookie.get('selected-path'));
    Vue.nextTick(function () {this.editor.refresh()}.bind(this))
  },


  ready: function () {
    this.editor = CodeMirror.fromTextArea(this.$els.textarea, {
      lineNumbers: true,
      mode: 'gcode'
    });
    this.doc = this.editor.getDoc();
    this.doc.on('change', this.change);

    var path = cookie.get('selected-path');
    if (!path) path = this.state.queued;
    this.load(path);

    window.addEventListener('resize', this.on_resize)
  },


  beforeDestroy: function () {
    window.removeEventListener('resize', this.on_resize)
  },


  methods: {
    on_resize: function () {
      // Scroll cursor into view if visible
      if (this.$el.offsetParent != null) this.editor.scrollIntoView(null)
    },


    change: function () {
      this.modified = !this.doc.isClean()

      var size = this.doc.historySize();
      this.canRedo = !!size.redo;
      this.canUndo = !!size.undo;
    },


    do_load: function (path, ok) {
      if (!path) this.set_path();
      else {
        this.loading = true;

        api.download('fs/' + path)
          .done(function (data) {
            this.set_path(path);
            this.set(data);
            this.loading = false;
            if (ok) ok();

          }.bind(this)).fail(function (text, xhr, status) {
            this.loading = false;
            this.$root.error_dialog('Failed to open <tt>' + path + '</tt>');
            if (cookie.get('selected-path') == path)
              cookie.set('selected-path', '');
          }.bind(this))
      }
    },


    load: function (path) {
      if (this.path == path) return;
      this.check_save(function () {this.do_load(path)}.bind(this));
    },


    set_path: function (path) {
      this.path = path;
      cookie.set('selected-path', path || '');
      this.editor.setOption('mode', util.get_highlight_mode(path))
    },


    set: function (text) {
      this.doc.setValue(text);
      this.doc.clearHistory();
      this.doc.markClean();
      this.modified = false;
      this.canRedo = false;
      this.canUndo = false;
    },


    check_save: function (ok) {
      if (!this.modified) return ok();

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
          save: function () {this.save(ok)}.bind(this),
          discard: function () {this.revert(ok)}.bind(this)
        }
      })
    },


    new_file: function () {
      this.check_save(function () {
        this.set_path();
        this.set('');
      }.bind(this));
    },


    open: function () {
      this.check_save(function () {
        this.$root.file_dialog({
          callback: function (path) {
            if (path) this.load(path)
          }.bind(this),
          dir: this.path ? util.dirname(this.path) : '/'
        })
      }.bind(this))
    },


    do_save: function (path, ok) {
      var fd = new FormData();
      var file = new File([new Blob([this.doc.getValue()])], path);
      fd.append('file', file);

      api.upload('fs/' + path, fd)
        .done(function () {
          this.set_path(path);
          this.modified = false;
          this.doc.markClean();
          if (typeof ok != 'undefined') ok()

        }.bind(this)).fail(function (error) {
          this.$root.error_dialog({body: 'Save failed'})
        }.bind(this));
    },


    save: function (ok) {
      if (!this.path) this.save_as(ok);
      else this.do_save(this.path, ok);
    },


    save_as: function (ok) {
      this.$root.file_dialog({
        save: true,
        callback: function (path) {
          if (path) this.do_save(path, ok);
        }.bind(this),
        dir: this.path ? util.dirname(this.path) : '/'
      })
    },


    revert: function (ok) {
      if (this.modified) {
        var path = this.path;
        this.path = undefined;
        this.modified = false;
        this.do_load(path, ok);
      }
    },


    download: function () {
      var data = new Blob([this.doc.getValue()], {type: 'text/plain'});
      window.URL.revokeObjectURL(this.$els.download.href);
      this.$els.download.href = window.URL.createObjectURL(data);
      this.$els.download.click();
    },


    view: function () {this.$root.view(this.path)},
    undo: function () {this.doc.undo()},
    redo: function () {this.doc.redo()},


    cut: function () {
      this.clipboard = this.doc.getSelection();
      this.doc.replaceSelection('');
    },


    copy: function () {this.clipboard = this.doc.getSelection()},
    paste: function () {this.doc.replaceSelection(this.clipboard)}
  }
}
