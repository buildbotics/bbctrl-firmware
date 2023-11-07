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
    text:     {type: String},
    icon:     {type: String},

    disabled: {
      type: Boolean,
      coerce(value) {return !!value}
    },
  },


  computed: {
    _icon() {
      if (this.icon) return this.icon

      switch (this.text.toLowerCase()) {
      case 'ok':     case 'yes': return 'check'
      case 'cancel': case 'no':  return 'times'
      case 'save':               return 'floppy-o'
      case 'discard':            return 'trash'
      }
    }
  },


  methods: {
    click(event) {
      if (!this.disabled) this.$emit('click', event)
    }
  }
}
