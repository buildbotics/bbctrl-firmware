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

let api = require('./api')


let kbd = {
  input: undefined,
  open: false,
  lockout: false,
  lost_focus: false,


  is_input: function(e) {
    var name = e.nodeName.toLowerCase()
    if (name == 'textarea') return true
    if (name == 'input' && !e.readOnly)
      return e.type == 'text' || e.type == 'number' || e.type == 'password'

    return false
  },


  hide: function() {
    kbd.lost_focus = true
    if (!kbd.open || kbd.lockout) return
    kbd.open = false
    api.put('keyboard/hide')
  },


  show: function() {
    kbd.lost_focus = false
    if (kbd.open) return
    kbd.open = true
    kbd.lockout = true
    api.put('keyboard/show')
  },


  click: function(e) {
    if (kbd.is_input(e.target)) kbd.input = e.target
    else if (kbd.lost_focus) {
      kbd.lockout = false
      kbd.hide()
    }
  },


  focusin: function(e) {
    if (kbd.is_input(e.target)) {
      kbd.input = e.target
      kbd.show()

    } else kbd.hide()
  },


  focusout: function (e) {
    if (kbd.is_input(e.target)) kbd.hide()
  },


  resize: function(e) {
    if (kbd.open) kbd.input.scrollIntoView({block: 'nearest'})
  },


  init: function () {
    if (location.host == 'localhost') {
      document.addEventListener('click',    kbd.click)
      document.addEventListener('focusin',  kbd.focusin)
      document.addEventListener('focusout', kbd.focusout)
      window  .addEventListener('resize',   kbd.resize)
    }
  }
}


kbd.init()
module.exports = kbd
