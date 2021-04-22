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

var api  = require('./api');
var util = require('./util');


function get_value(value, file, defaultValue) {
  if (typeof value == 'function') value = value(file);
  if (value == undefined) value = defaultValue;
  return value;
}


module.exports = {
  template: '#upload-dialog-template',


  data: function () {
    return {
      config: {},
      show: false,
      progress: 0,
      msg: ''
    }
  },


  methods: {
    upload: function (config)  {
      this.config = config;

      // Reset <form> so the browser does not cache the file when the name is
      // same but the contents have changed.
      this.$els.form.reset();

      var input = this.$els.input;
      if (config.multiple) input.setAttribute('multiple', '');
      else input.removeAttribute('multiple');
      input.setAttribute('accept', get_value(config.accept, undefined, ''));
      input.click();
    },


    cancel: function () {
      this.canceled = true;
      if (this.xhr) this.xhr.abort();
    },


    _upload_file: function (file, next) {
      var _progress = function (fraction, done, xhr) {
        if (typeof this.config.on_progress == 'function')
          this.config.on_progress(fraction, done, xhr);

        this.xhr = xhr;
        this.progress = (fraction * 100).toFixed(1);
      }.bind(this);

      this.progress = 0;
      this.show = true;

      this.msg = get_value(this.config.msg,  file, file.name);
      var url  = get_value(this.config.url,  file);
      var form = get_value(this.config.form, file, {file: file});
      var data = get_value(this.config.data, file, {});

      var fd = new FormData();
      for (const key in form)
        fd.append(key, get_value(form[key], file));

      api.upload(url, fd, data, _progress)
        .done(function () {
          if (typeof this.config.on_success == 'function')
            this.config.on_success(file, next);
          else next();

        }.bind(this)).fail(function (error) {
          this.show = false;
          if (this.canceled) return;
          if (typeof this.config.on_failure == 'function')
            this.config.on_failure(file, error);
          else this.$root.api_error(this.msg + ' failed', error)
        }.bind(this));
    },


    _upload_files: function (files, i) {
      if (files.length <= i || this.canceled) return this.show = false;
      var file = files[i];

      var _next   = function () {this._upload_files(files, i + 1)}.bind(this);
      var _upload = function () {this._upload_file(file,   _next)}.bind(this);

      if (typeof this.config.on_confirm == 'function')
        this.config.on_confirm(file, function (ok) {
          if (ok) _upload();
          else _next();
        }.bind(this));

      else _upload();
    },


    _upload: function (e)  {
      this.canceled = false;
      this._upload_files(e.target.files || e.dataTransfer.files, 0);
    }
  }
}
