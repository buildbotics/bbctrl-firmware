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

var api    = require('./api');
var cookie = require('./cookie');
var util   = require('./util');


module.exports = {
  template: '#view-viewer-template',
  props: ['config', 'template', 'state'],


  data: function () {
    return {
      loading: false,
      retry: 0,
      path: undefined,
      toolpath: {},
      progress: 0,
      snaps: 'angled top bottom front back left right'.split(' ')
    }
  },


  components: {
    'viewer-help-dialog': require('./viewer-help-dialog'),
    'path-viewer':        require('./path-viewer')
  },


  computed: {
    filename: function () {return util.display_path(this.path)},


    show: function () {
      if (this.$refs.viewer == undefined) return {};
      return this.$refs.viewer.show;
    }
  },


  watch: {
    'state.queued': function () {
      if (!this.path && this.state.queued) this.load(this.state.queued);
    }
  },


  attached: function () {
    var path = cookie.get('selected-path');
    if (!path) path = this.state.queued;
    this.load(path)
  },


  methods: {
    _load: function (path) {
      this.loading = true;

      api.get('path/' + path).done(function (toolpath) {
        if (path != this.path) return;
        this.retry = 0;

        if (toolpath.progress == undefined) {
          toolpath.path = path;
          this.progress = 1;
          this.toolpath = toolpath;
          this.loading = false;

        } else {
          this._load(path); // Try again
          this.progress = toolpath.progress;
        }

      }.bind(this)).fail(function (error, xhr) {
        if (xhr.status == 404) {
          this.loading = false;
          this.$root.api_error('', error);
          return
        }

        if (++this.retry < 10)
          setTimeout(function () {this._load(path)}.bind(this), 5000);
        else {
          this.loading = false;
          this.$root.api_error('3D view loading failed', error);
        }
      }.bind(this))
    },


    load: function(path) {
      //if (!path || this.path == path) return;
      if (!path) return;

      cookie.set('selected-path', path)
      this.path = path;
      this.progress = 0;
      this.toolpath = {};
      this.$refs.viewer.clear();

      if (path) this._load(path);
    },


    open: function () {
      this.$root.file_dialog({
        callback: function (path) {this.load(path)}.bind(this),
        dir: util.dirname(this.path)
      })
    },


    toggle: function (name) {this.$refs.viewer.toggle(name)},
    snap: function (view) {this.$refs.viewer.snap(view)}
  }
}
