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


module.exports = {
  template: '#bbutton-template',


  props: {
    text:      {type: String},
    icon:      {type: String},
    success:   {type: Boolean, default: false},
    primary:   {type: Boolean, default: false},
    secondary: {type: Boolean, default: false},

    disabled: {
      type: Boolean,
      coerce(value) {return !!value}
    },
  },


  computed: {
    classes() {
      return {
        success:                 this.success,
        'pure-button-primary':   this.primary,
        'pure-button-secondary': this.secondary,
      }
    },


    _icon() {
      if (this.icon) return this.icon
      if (!this.text) return

      let text = this.text.toLowerCase()
      switch (text) {
      case 'ok':     case 'yes': return 'check'
      case 'cancel': case 'no':  return 'times'
      case 'save':               return 'floppy-o'
      case 'open':               return 'folder-open'
      case 'discard':            return 'trash'
      case 'add': case 'create': return 'plus'
      case 'edit':               return 'pencil'
      case 'login':              return 'sign-in'
      case 'logout':             return 'sign-out'
      }

      return text
    }
  }
}
