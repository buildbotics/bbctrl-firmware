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


function make_buttons(buttons) {
  if (typeof buttons == 'string') buttons = buttons.split(' ')

  let hasClass = false
  for (let i = 0; i < buttons.length; i++) {
    if (typeof buttons[i] == 'string') buttons[i] = {text: buttons[i]}
    buttons[i].action = buttons[i].action || buttons[i].text.toLowerCase()
    if (buttons[i].class) hasClass = true
  }

  if (buttons.length && !hasClass)
    buttons.slice(-1)[0].class = 'pure-button-primary'

  return buttons
}


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
      coerce(buttons) {return make_buttons(buttons)}
    }
  },


  data() {
    return {
      show: false
    }
  },


  methods: {
    click_away() {if (this.clickAway) this.close('cancel')},


    close(action) {
      Vue.nextTick(() => {this.show = false})

      if (this.resolve != undefined) {
        this.resolve(action)
        delete this.resolve
      }

      this.$emit('closed', action)
    },


    open(config) {
      if (config)
        for (let name in config) {
          if (name == 'buttons') this[name] = make_buttons(config[name])
          else this[name] = config[name]
        }

      this.show = true
      Vue.nextTick(this.focus)

      return new Promise(resolve => this.resolve = resolve)
    },


    async _message(icon, header, body) {
      return this.open({icon, header, body, buttons: 'Ok'})
    },


    async success(msg) {return this._message('check',       'Success', msg)},
    async error(msg)   {return this._message('exclamation', 'Error',   msg)},


    async warning(msg) {
      return this._message('exclamation-triangle', 'Warning', msg)
    },


    blur() {
      $(this.$el).find('[focus]').each((index, e) => {
        e.blur()
        return false
      })
    },


    focus() {
      $(this.$el).find('[focus]').each((index, e) => {
        e.focus()
        return false
      })
    }
  }
}
