/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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


module.exports = class Keyboard {
  constructor(api) {
    this.api   = api
    this.input = undefined
    this.open  = false
  }


  install() {
    document.addEventListener('focusin',  e => this.focusin(e))
    document.addEventListener('focusout', e => this.focusout(e))
    window  .addEventListener('resize',   e => this.resize(e))
  }


  is_input(e) {
    let name = e.nodeName.toLowerCase()
    if (name == 'textarea') return true
    if (name == 'input' && !e.readOnly)
      return e.type == 'text' || e.type == 'number' || e.type == 'password'

    return false
  }


  async hide(force) {
    if (!this.open) return
    this.open = false
    return this.api.put('keyboard/hide')
  }


  async show() {
    if (this.open) return
    this.open = true
    return this.api.put('keyboard/show')
  }


  focusin(e) {
    if (this.is_input(e.target)) {
      this.input = e.target
      this.show()

    } else this.hide()
  }


  focusout(e) {if (this.is_input(e.target)) this.hide()}
  resize(e) {if (this.open) this.input.scrollIntoView({block: 'nearest'})}
}
