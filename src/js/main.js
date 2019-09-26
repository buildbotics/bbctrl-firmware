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

'use strict';


function cookie_get(name) {
  var decodedCookie = decodeURIComponent(document.cookie);
  var ca = decodedCookie.split(';');
  name = name + '=';

  for (var i = 0; i < ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') c = c.substring(1);
    if (!c.indexOf(name)) return c.substring(name.length, c.length);
  }
}


function cookie_set(name, value, days) {
  var d = new Date();
  d.setTime(d.getTime() + days * 24 * 60 * 60 * 1000);
  var expires = 'expires=' + d.toUTCString();
  document.cookie = name + '=' + value + ';' + expires + ';path=/';
}


var uuid_chars =
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+';


function uuid(length) {
  if (typeof length == 'undefined') length = 52;

  var s = '';
  for (var i = 0; i < length; i++)
    s += uuid_chars[Math.floor(Math.random() * uuid_chars.length)];

  return s
}


$(function() {
  if (typeof cookie_get('client-id') == 'undefined')
    cookie_set('client-id', uuid(), 10000);

  // Vue debugging
  Vue.config.debug = true;
  //Vue.util.warn = function (msg) {console.debug('[Vue warn]: ' + msg)}

  // Register global components
  Vue.component('templated-input', require('./templated-input'));
  Vue.component('message', require('./message'));
  Vue.component('indicators', require('./indicators'));
  Vue.component('io-indicator', require('./io-indicator'));
  Vue.component('console', require('./console'));
  Vue.component('unit-value', require('./unit-value'));

  Vue.filter('number', function (value) {
    if (isNaN(value)) return 'NaN';
    return value.toLocaleString();
  });

  Vue.filter('percent', function (value, precision) {
    if (typeof value == 'undefined') return '';
    if (typeof precision == 'undefined') precision = 2;
    return (value * 100.0).toFixed(precision) + '%';
  });

  Vue.filter('non_zero_percent', function (value, precision) {
    if (!value) return '';
    if (typeof precision == 'undefined') precision = 2;
    return (value * 100.0).toFixed(precision) + '%';
  });

  Vue.filter('fixed', function (value, precision) {
    if (typeof value == 'undefined') return '0';
    return parseFloat(value).toFixed(precision)
  });

  Vue.filter('upper', function (value) {
    if (typeof value == 'undefined') return '';
    return value.toUpperCase()
  });

  Vue.filter('time', function (value, precision) {
    if (isNaN(value)) return '';
    if (isNaN(precision)) precision = 0;

    var MIN = 60;
    var HR  = MIN * 60;
    var DAY = HR * 24;
    var parts = [];

    if (DAY <= value) {
      parts.push(Math.floor(value / DAY));
      value %= DAY;
    }

    if (HR <= value) {
      parts.push(Math.floor(value / HR));
      value %= HR;
    }

    if (MIN <= value) {
      parts.push(Math.floor(value / MIN));
      value %= MIN;

    } else parts.push(0);

    parts.push(value);

    for (var i = 0; i < parts.length; i++) {
      parts[i] = parts[i].toFixed(i == parts.length - 1 ? precision : 0);
      if (i && parts[i] < 10) parts[i] = '0' + parts[i];
    }

    return parts.join(':');
  });

  // Vue app
  require('./app');
});
