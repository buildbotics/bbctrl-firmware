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


function get_icon(action) {
  switch (action.toLowerCase()) {
  case 'ok':     case 'yes': return 'check';
  case 'cancel': case 'no':  return 'times';
  case 'save':               return 'floppy-o';
  case 'discard':            return 'trash';
  }

  return undefined
}


function make_button(button) {
  if (typeof button == 'string') button = {text: button}
  button.action = button.action || button.text.toLowerCase()
  button.icon = button.icon || get_icon(button.action);
  return button;
}


module.exports = {
  template: '#dialog-template',


  data: function () {
    return {
      show: false,
      config: {},
      buttons: []
    }
  },


  methods: {
    click_away: function () {
      if (typeof this.config.click_away == 'undefined')
        this.close('click-away');

      if (this.config.click_away) this.close(this.config.click_away);
    },


    close: function (action) {
      this.show = false

      if (typeof this.config.callback == 'function')
        this.config.callback(action);

      if (typeof this.config.callback == 'object' &&
          typeof this.config.callback[action] == 'function')
        this.config.callback[action]();
    },


    open: function(config) {
      this.config = config;

      var buttons = config.buttons || 'OK';
      if (typeof buttons == 'string') buttons = buttons.split(' ');

      this.buttons = [];
      for (var i = 0; i < buttons.length; i++)
        this.buttons.push(make_button(buttons[i]))

      this.show = true;
    },


    error: function (msg) {
      this.open({
        icon: 'exclamation',
        title: 'Error',
        body: msg
      })
    },


    warning: function (msg) {
      this.open({
        icon: 'exclamation-triangle',
        title: 'Warning',
        body: msg
      })
    },


    success: function (msg) {
      this.open({
        icon: 'check',
        title: 'Success',
        body: msg
      })
    }
  }
}
