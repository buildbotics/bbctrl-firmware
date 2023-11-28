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



let cookie = {
  prefix: 'bbctrl-',


  get(name, defaultValue) {
    let decodedCookie = decodeURIComponent(document.cookie)
    let ca = decodedCookie.split(';')
    name = cookie.prefix + name + '='

    for (let i = 0; i < ca.length; i++) {
      let c = ca[i]
      while (c.charAt(0) == ' ') c = c.substring(1)
      if (!c.indexOf(name)) return c.substring(name.length, c.length)
    }

    return defaultValue
  },


  set(name, value, days) {
    let offset = 2147483647 // Max value
    if (typeof days != 'undefined') offset = days * 24 * 60 * 60 * 1000
    let d = new Date()
    d.setTime(d.getTime() + offset)
    let expires = 'expires=' + d.toUTCString()
    document.cookie =
      cookie.prefix + name + '=' + value + ';' + expires + ';path=/;samesite=lax'
  },


  set_bool(name, value) {
    cookie.set(name, value ? 'true' : 'false')
  },


  get_bool(name, defaultValue) {
    return cookie.get(name, defaultValue ? 'true' : 'false') == 'true'
  }
}


module.exports = cookie
