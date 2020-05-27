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

'use strict'


function _msg_equal(a, b) {
  return a.level == b.level && a.source == b.source && a.where == b.where &&
    a.msg == b.msg;
}


// Shared among all instances
var messages = [];


module.exports = {
  template: '#console-template',


  data: function () {
    return {messages: messages}
  },


  events: {
    log: function (msg) {
      // There may be multiple instances of this module so ignore messages
      // that have already been processed.
      if (msg.logged) return;
      msg.logged = true;

      // Make sure we have a message level
      msg.level = msg.level || 'info';

      // Add to message log and count and collapse repeats
      var repeat = messages.length && _msg_equal(msg, messages[0]);
      if (repeat) messages[0].repeat++;
      else {
        msg.repeat = msg.repeat || 1;
        messages.unshift(msg);
        while (256 < messages.length) messages.pop();
      }
      msg.ts = Date.now();

      // Write message to browser console for debugging
      var text = JSON.stringify(msg);
      if (msg.level == 'error' || msg.level == 'critical') console.error(text);
      else if (msg.level == 'warning') console.warn(text);
      else if (msg.level == 'debug' && console.debug) console.debug(text);
      else console.log(text);

      // Event on errors
      if (msg.level == 'error' || msg.level == 'critical')
        this.$dispatch('error', msg);
    }
  },


  methods: {
    clear: function () {messages.splice(0, messages.length);},
  }
}
