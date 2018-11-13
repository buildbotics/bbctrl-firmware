/******************************************************************************\

                    Copyright 2018. Buildbotics LLC
                              All Rights Reserved.

                  For information regarding this software email:
                                 Joseph Coffland
                              joseph@buildbotics.com

        This software is free software: you clan redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        This software is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

\******************************************************************************/

'use strict'


module.exports = function (prefix) {
  if (typeof prefix == 'undefined') prefix = '';

  var cookie = {
    get: function (name, defaultValue) {
      var decodedCookie = decodeURIComponent(document.cookie);
      var ca = decodedCookie.split(';');
      name = prefix + name + '=';

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
      document.cookie = prefix + name + '=' + value + ';' + expires + ';path=/';
    },


    set_bool: function (name, value) {
      cookie.set(name, value ? 'true' : 'false');
    },


    get_bool: function (name, defaultValue) {
      return cookie.get(name, defaultValue ? 'true' : 'false') == 'true';
    }
  }

  return cookie;
}
