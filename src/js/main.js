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


var cookie = require('./cookie');
var util   = require('./util');
var api    = require('./api');


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


var kbd_input = undefined;
var kbd_open = false;


function keyboard_hide() {
  if (!kbd_open) return
  kbd_open = false
  api.put('keyboard/hide')
}


function keyboard_show() {
  if (kbd_open) return
  kbd_open = true
  api.put('keyboard/show')
}


function is_input(e) {
  var name = e.nodeName.toLowerCase()
  if (name == 'textarea') return true
  if (name == 'input' && !e.readOnly)
    return e.type == 'text' || e.type == 'number' || e.type == 'password'
  return false
}


function keyboard_click(e) {kbd_input = e.target}


function keyboard_in(e) {
  if (is_input(e.target)) keyboard_show();
  else keyboard_hide(e);
}


function keyboard_out(e) {
  if (is_input(e.target)) keyboard_hide();
}


function keyboard_resize(e) {
  if (kbd_open) kbd_input.scrollIntoView({block: 'nearest'})
}


$(function() {
  if (location.host == 'localhost') {
    document.addEventListener('click', keyboard_click);
    document.addEventListener('focusin', keyboard_in);
    document.addEventListener('focusout', keyboard_out);
    window.addEventListener('resize', keyboard_resize);
  }

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
