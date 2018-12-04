/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

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
      no_data_text: 'GCode viewer...',
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
