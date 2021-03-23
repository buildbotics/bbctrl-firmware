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

'use strict';


var util = {
  SEC_PER_YEAR:  365 * 24 * 60 * 60,
  SEC_PER_MONTH: 30 * 24 * 60 * 60,
  SEC_PER_WEEK:  7 * 24 * 60 * 60,
  SEC_PER_DAY:   24 * 60 * 60,
  SEC_PER_HOUR:  60 * 60,
  SEC_PER_MIN:   60,
  uuid_chars:
  'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+',


  duration: function (x, name, precision) {
    x = x.toFixed(typeof precision == 'undefined' ? 0 : precision);
    return x + ' ' + name + (x == 1 ? '' : 's')
  },


  human_duration: function (x, precision) {
    if (util.SEC_PER_YEAR <= x)
      return util.duration(x / util.SEC_PER_YEAR,  'year',  precision);
    if (util.SEC_PER_MONTH <= x)
      return util.duration(x / util.SEC_PER_MONTH, 'month', precision);
    if (util.SEC_PER_WEEK <= x)
      return util.duration(x / util.SEC_PER_WEEK,  'week',  precision);
    if (util.SEC_PER_DAY <= x)
      return util.duration(x / util.SEC_PER_DAY,   'day',   precision);
    if (util.SEC_PER_HOUR <= x)
      return util.duration(x / util.SEC_PER_HOUR,  'hour',  precision);
    if (util.SEC_PER_MIN <= x)
      return util.duration(x / util.SEC_PER_MIN,   'min',   precision);
    return util.duration(x, 'sec', precision);
  },


  human_size: function (x, precision) {
    if (typeof precision == 'undefined') precision = 1;

    if (1e12 <= x) return (x / 1e12).toFixed(precision) + 'T'
    if (1e9  <= x) return (x / 1e9 ).toFixed(precision) + 'B'
    if (1e6  <= x) return (x / 1e6 ).toFixed(precision) + 'M'
    if (1e3  <= x) return (x / 1e3 ).toFixed(precision) + 'K'
    return x;
  },


  unix_path: function (path) {
    if (/Win/i.test(navigator.platform)) return path.replace('\\', '/');
    return path;
  },


  dirname: function (path) {
    var sep = path.lastIndexOf('/');
    return sep == -1 ? '.' : (sep == 0 ? '/' : path.substr(0, sep));
  },


  basename: function (path) {return path.substr(path.lastIndexOf('/') + 1)},


  join_path: function (a, b) {
    if (!a) return b;
    return a[a.length - 1] == '/' ? a + b : (a + '/' + b);
  },


  display_path: function (path) {
    if (path == undefined) return path;
    return path.startsWith('Home/') ? path.substr(5) : path;
  },


  uuid: function (length) {
    if (typeof length == 'undefined') length = 52;

    var s = '';
    for (var i = 0; i < length; i++)
      s += util.uuid_chars[Math.floor(Math.random() * util.uuid_chars.length)];

    return s
  },


  get_highlight_mode: function (path) {
    if (path.endsWith('.tpl') || path.endsWith('.js') ||
        path.endsWith('.json'))
      return 'javascript';

    return 'gcode'
  },


  webgl_supported: function (performance) {
    try {
      var opts = {failIfMajorPerformanceCaveat: performance || false};
      var canvas = document.createElement('canvas');
      return !!(window.WebGLRenderingContext &&
                (canvas.getContext('webgl', opts) ||
                 canvas.getContext('experimental-webgl', opts)));

    } catch (e) {return false}
  }
}


module.exports = util;
