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

'use strict';


var cookie = require('./cookie');
var util   = require('./util');


function menu_ui() {
  var layout   = document.getElementById('layout');
  var menuLink = document.getElementById('menuLink');

  var collapse = function () {
    layout.classList.remove('active');
    window.removeEventListener('click', collapse);
  }

  menuLink.onclick = function (e) {
    e.preventDefault();
    e.stopPropagation();
    layout.classList.toggle('active');
    window.addEventListener('click', collapse);
  }
}


$(function() {
  menu_ui();

  if (typeof cookie.get('client-id') == 'undefined')
    cookie.set('client-id', util.uuid());

  // Vue debugging
  Vue.config.debug = true;

  // CodeMirror GCode mode
  require('./cm-gcode');

  // Register global components
  Vue.component('templated-input', require('./templated-input'));
  Vue.component('message',         require('./message'));
  Vue.component('loading-message', require('./loading-message'));
  Vue.component('dialog',          require('./dialog'));
  Vue.component('indicators',      require('./indicators'));
  Vue.component('io-indicator',    require('./io-indicator'));
  Vue.component('console',         require('./console'));
  Vue.component('unit-value',      require('./unit-value'));
  Vue.component('files',           require('./files'));
  Vue.component('file-dialog',     require('./file-dialog'));
  Vue.component('upload-dialog',   require('./upload-dialog'));
  Vue.component('nav-menu',        require('./nav-menu'));
  Vue.component('nav-item',        require('./nav-item'));
  Vue.component('video',           require('./video'));
  Vue.component('color-picker',    require('./color-picker'));
  Vue.component('dragbar',         require('./dragbar'));

  require('./filters')();

  // Vue app
  require('./app');
});
