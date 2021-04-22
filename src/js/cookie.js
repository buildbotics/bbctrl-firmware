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


var cookie = {
  prefix: 'bbctrl-',


  get: function (name, defaultValue) {
    var decodedCookie = decodeURIComponent(document.cookie);
    var ca = decodedCookie.split(';');
    name = cookie.prefix + name + '=';

    for (var i = 0; i < ca.length; i++) {
      var c = ca[i];
      while (c.charAt(0) == ' ') c = c.substring(1);
      if (!c.indexOf(name)) return c.substring(name.length, c.length);
    }

    return defaultValue;
  },


  set: function (name, value, days) {
    var offset = 2147483647; // Max value
    if (typeof days != 'undefined') offset = days * 24 * 60 * 60 * 1000;
    var d = new Date();
    d.setTime(d.getTime() + offset);
    var expires = 'expires=' + d.toUTCString();
    document.cookie =
      cookie.prefix + name + '=' + value + ';' + expires + ';path=/';
  },


  set_bool: function (name, value) {
    cookie.set(name, value ? 'true' : 'false');
  },


  get_bool: function (name, defaultValue) {
    return cookie.get(name, defaultValue ? 'true' : 'false') == 'true';
  }
}


module.exports = cookie;
