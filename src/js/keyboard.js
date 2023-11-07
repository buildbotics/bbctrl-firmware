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


let kbd = {
  input: undefined,
  open: false,
  lockout: false,
  lost_focus: false,


  is_input(e) {
    let name = e.nodeName.toLowerCase()
    if (name == 'textarea') return true
    if (name == 'input' && !e.readOnly)
      return e.type == 'text' || e.type == 'number' || e.type == 'password'

    return false
  },


  async hide() {
    kbd.lost_focus = true
    if (!kbd.open || kbd.lockout) return
    kbd.open = false
    return this.$api.put('keyboard/hide')
  },


  async show() {
    kbd.lost_focus = false
    if (kbd.open) return
    kbd.open = true
    kbd.lockout = true
    return this.$api.put('keyboard/show')
  },


  click(e) {
    if (kbd.is_input(e.target)) kbd.input = e.target
    else if (kbd.lost_focus) {
      kbd.lockout = false
      kbd.hide()
    }
  },


  focusin(e) {
    if (kbd.is_input(e.target)) {
      kbd.input = e.target
      kbd.show()

    } else kbd.hide()
  },


  focusout(e) {
    if (kbd.is_input(e.target)) kbd.hide()
  },


  resize(e) {
    if (kbd.open) kbd.input.scrollIntoView({block: 'nearest'})
  },


  init() {
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
