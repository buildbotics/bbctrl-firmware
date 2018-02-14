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
    message: function (msg) {
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
      }

      // Write message to browser console for debugging
      var text = JSON.stringify(msg);
      if (msg.level == 'error' || msg.level == 'critical') console.error(text);
      else if (msg.level == 'warning') console.warn(text);
      else if (msg.level == 'debug' && console.debug) console.debug(text);
      else console.log(text);

      // Event on errors
      if (!repeat && (msg.level == 'error' || msg.level == 'critical'))
        this.$dispatch('error', msg);
    }
  },


  methods: {
    clear: function () {messages.length = 0;},
  }
}
