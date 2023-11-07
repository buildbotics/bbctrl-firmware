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
  template: '#dialog-template',


  props: {
    width:     {type: String,  default: '300px'},
    icon:      {type: String},
    clickAway: {type: Boolean, default: true},
    header:    {type: String,  default: ''},
    body:      {type: String,  default: ''},

    buttons: {
      type: Array,
      default() {return 'Ok'},


      coerce(buttons) {
        if (typeof buttons == 'string') buttons = buttons.split(' ')

        for (let i = 0; i < buttons.length; i++) {
          if (typeof buttons[i] == 'string') buttons[i] = {text: buttons[i]}
          buttons[i].action = buttons[i].action || buttons[i].text.toLowerCase()
        }

        return buttons
      }
    }
  },


  data() {
    return {
      show: false
    }
  },


  methods: {
    close(action) {
      this.show = false
      this.$emit('close', action)

      if (this.resolve) {
        this.resolve(action)
        delete this.resolve
      }
    },


    open(config) {
      if (config)
        for (let name in config)
          this[name] = config[name]

      this.show = true
      return new Promise(resolve => this.resolve = resolve)
    },


    async error(msg) {
      return this.open({icon: 'exclamation', header: 'Error', body: msg})
    },


    async warning(msg) {
      return this.open({
        icon: 'exclamation-triangle',
        header: 'Warning',
        body: msg
      })
    },


    async success(msg) {
      return this.open({icon: 'check', header: 'Success', body: msg})
    }
  }
}
