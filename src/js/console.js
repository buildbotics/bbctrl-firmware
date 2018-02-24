/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

'use strict'


function _msg_equal(a, b) {
  return a.level == b.level && a.location == b.location && a.code == b.code &&
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
        msg.repeat = 1;
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
