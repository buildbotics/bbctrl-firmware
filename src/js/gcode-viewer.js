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

var api = require('./api');


var entityMap = {
  '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  '/': '&#x2F;', '`': '&#x60;', '=': '&#x3D;'}


function escapeHTML(s) {
  return s.replace(/[&<>"'`=\/]/g, function (c) {return entityMap[c]})
}


module.exports = {
  template: '#gcode-viewer-template',


  data: function () {
    return {
      empty: true,
      file: '',
      line: -1,
      scrolling: false
    }
  },


  events: {
    'gcode-load': function (file) {this.load(file)},
    'gcode-clear': function () {this.clear()},
    'gcode-reload': function (file) {this.reload(file)},
    'gcode-line': function (line) {this.update_line(line)}
  },


  ready: function () {
    this.clusterize = new Clusterize({
      rows: [],
      scrollElem: $(this.$el).find('.clusterize-scroll')[0],
      contentElem: $(this.$el).find('.clusterize-content')[0],
      no_data_text: 'GCode view...',
      callbacks: {clusterChanged: this.highlight}
    });
  },


  attached: function () {
    if (typeof this.clusterize != 'undefined')
      this.clusterize.refresh(true);
  },


  methods: {
    load: function (file) {
      if (file == this.file) return;
      this.clear();
      this.file = file;

      if (!file) return;

      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/api/file/' + file + '?' + Math.random(), true);
      xhr.responseType = 'text';

      xhr.onload = function (e) {
        if (this.file != file) return;
        var lines = escapeHTML(xhr.response.trimRight()).split(/\r?\n/);

        for (var i = 0; i < lines.length; i++) {
          lines[i] = '<li class="ln' + (i + 1) + '">' +
            '<b>' + (i + 1) + '</b>' + lines[i] + '</li>';
        }

        this.clusterize.update(lines);
        this.empty = false;

        Vue.nextTick(this.update_line);
      }.bind(this)

      xhr.send();
    },


    clear: function () {
      this.empty = true;
      this.file = '';
      this.line = -1;
      this.clusterize.clear();
    },


    reload: function (file) {
      if (file != this.file) return;
      this.clear();
      this.load(file);
    },


    highlight: function () {
      var e = $(this.$el).find('.highlight');
      if (e.length) e.removeClass('highlight');

      e = $(this.$el).find('.ln' + this.line);
      if (e.length) e.addClass('highlight');
    },


    update_line: function(line) {
      if (typeof line != 'undefined') {
        if (this.line == line) return;
        this.line = line;

      } else line = this.line;

      var totalLines = this.clusterize.getRowsAmount();

      if (line <= 0) line = 1;
      if (totalLines < line) line = totalLines;

      var e = $(this.$el).find('.clusterize-scroll');

      var lineHeight = e[0].scrollHeight / totalLines;
      var linesPerPage = Math.floor(e[0].clientHeight / lineHeight);
      var current = e[0].scrollTop / lineHeight;
      var target = line - 1 - Math.floor(linesPerPage / 2);

      // Update scroll position
      if (!this.scrolling) {
        if (target < current - 20 || current + 20 < target)
          e[0].scrollTop = target * lineHeight;

        else {
          this.scrolling = true;
          e.animate({scrollTop: target * lineHeight}, {
            complete: function () {this.scrolling = false}.bind(this)
          })
        }
      }

      Vue.nextTick(this.highlight);
    }
  }
}
