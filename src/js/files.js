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

var api  = require('./api')
var util = require('./util')


function order_files(a, b) {
  if (a.dir != b.dir) return a.dir ? -1 : 1;
  return a.name.localeCompare(b.name);
}


function valid_filename(name) {
  return name.length && name[0] != '.' && name.indexOf('/') == -1;
}


module.exports = {
  template: '#files-template',
  props: ['mode', 'locations'],


  data: function () {
    return {
      fs: {},
      selected: -1,
      filename: '',
      folder: '',
      showNewFolder: false
    }
  },


  watch: {
    selected: function () {
      var path;
      var dir = false;

      if (0 <= this.selected && this.selected <= this.files.length) {
        var file = this.files[this.selected];
        if (file.dir) dir = true;
        path = this.file_path(file);
      }

      if (this.mode != 'save') this.$emit('selected', path, dir);
      if (path && !dir) this.filename = util.basename(path);
    },


    filename: function () {
      if (this.mode != 'save') return;
      var path;
      if (this.filename_valid)
        path = util.join_path(this.fs.path, this.filename)
      this.$emit('selected', path, false);
    },


    locations: function () {
      if (this.locations.indexOf(this.location) == -1)
        this.load('')
    }
  },


  computed: {
    files: function () {
      if (typeof this.fs.files == 'undefined') return [];
      return this.fs.files.sort(order_files);
    },


    location: function () {
      if (typeof this.fs.path != 'undefined') {
        var paths = this.fs.path.split('/').filter(function (s) {return s});
        if (paths.length) return paths[0];
      }

      return 'Home';
    },


    paths: function () {
      if (typeof this.fs.path == 'undefined') return [];

      var paths = this.fs.path.split('/').filter(function (s) {return s});
      if (paths.length) paths.shift(); // Remove location
      paths.unshift('/');

      return paths;
    },


    folder_valid: function () {
      var file = this.find_file(this.folder);
      return file == undefined && valid_filename(this.folder);
    },


    filename_valid: function () {
      var file = this.find_file(this.filename);
      return (file == undefined || !file.dir) && valid_filename(this.filename);
    }
  },


  ready: function () {this.load('')},


  methods: {
    location_title: function (name) {
      if (name == 'Home')
        return 'Select files already on the controller.';
      return 'Select files from a USB drive.';
    },


    filename_changed: function () {
      if (this.selected != -1 &&
          this.filename != this.files[this.selected].name)
        this.selected = -1;
    },


    find_file: function (name) {
      for (var i = 0; i < this.files.length; i++)
        if (this.files[i].name == name) return this.files[i];
      return undefined;
    },


    has_file:  function (name) {return this.find_file(name) != undefined},
    file_path: function (file) {return util.join_path(this.fs.path, file.name)},
    file_url:  function (file) {return '/api/fs' + this.file_path(file)},
    select:    function (index) {this.selected = index},


    eject: function (location) {
      api.put('usb/eject/' + location)
    },


    open: function (path) {
      this.filename = '';
      this.load(path);
    },


    load: function (path, done) {
      api.get('fs/' + path)
        .done(function (data) {
          this.fs = data
          this.selected = -1;
          if (done) done();
        }.bind(this))
    },


    reload: function (done) {this.load(this.fs.path || '', done)},


    path_at: function (index) {
      return '/' + this.paths.slice(1, index + 1).join('/');
    },


    path_title: function (index) {
      if (index == this.paths.length - 1) return '';
      return 'Go to folder ' + this.path_at(index);
    },


    load_path: function (index) {
      this.load(this.location + this.path_at(index))
    },


    new_folder: function () {
      this.folder = '';
      this.showNewFolder = true;
    },


    create_folder: function () {
      if (!this.folder_valid) return;
      this.showNewFolder = false;

      var path = this.fs.path + '/' + this.folder;

      api.put('fs/' + path)
        .done(function () {this.open(path)}.bind(this));
    },


    activate: function (file) {
      if (file.dir) this.load(this.fs.path + '/' + file.name);
      else this.$emit('activate', this.file_path(file));
    },


    delete: function (file) {
      this.$root.open_dialog({
        title: 'Delete ' + (file.dir ? 'directory' : 'file') + '?',
        body: 'Are you sure you want to delete <tt>' + file.name +
          (file.dir ? '</tt> and all the files under it?' : '</tt>?'),
        buttons: 'Cancel OK',
        callback: function (action) {
          if (action == 'ok')
            api.delete('fs/' + this.fs.path + '/' + file.name)
            .done(this.reload)
        }.bind(this)
      });
    },


    activate_file: function (filename) {
      for (var i = 0; i < this.files.length; i++)
        if (this.files[i].name == filename)
          this.activate(this.files[i]);
    },


    confirm_upload: function (file, cb) {
      file.filename = util.basename(util.unix_path(file.name));

      var other = this.find_file(file.filename);
      if (!other) return cb(true);

      if (other.dir)
        this.$root.open_dialog({
          title: 'Cannot overwrite',
          body: 'Cannot overwrite directory ' + file.filename + '.',
          buttons: 'OK',
          callback: function () {cb(false)}
        });

      else
        this.$root.open_dialog({
          title: 'Overwrite file?',
          body: 'Are you sure you want to overwrite ' + file.filename + '?',
          buttons: 'Cancel OK',
          callback: function (action) {cb(action == 'ok')}
        });
    },


    upload_success: function (file, next) {
      this.reload(function () {
        if (this.mode == 'open') this.activate_file(file.filename)
        next();
      }.bind(this));
    },


    upload: function ()  {
      this.$root.upload({
        multiple: this.mode != 'open',
        url: function (file) {
          return 'fs/' + this.fs.path + '/' + file.filename;
        }.bind(this),
        on_confirm: this.confirm_upload,
        on_success: this.upload_success
      })
    }
  }
}
