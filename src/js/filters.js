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

'use strict';

var util = require('./util');


var filters = {
  number: function (value) {
    if (isNaN(value)) return 'NaN';
    return value.toLocaleString();
  },


  percent: function (value, precision) {
    if (typeof value == 'undefined') return '';
    if (typeof precision == 'undefined') precision = 2;
    return (value * 100.0).toFixed(precision) + '%';
  },



  non_zero_percent: function (value, precision) {
    if (!value) return '';
    if (typeof precision == 'undefined') precision = 2;
    return (value * 100.0).toFixed(precision) + '%';
  },


  fixed: function (value, precision) {
    if (typeof value == 'undefined') return '0';
    return parseFloat(value).toFixed(precision)
  },


  upper: function (value) {
    if (typeof value == 'undefined') return '';
    return value.toUpperCase()
  },


  time: function (value, precision) {
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
  },


  ago: function (ts) {
    if (typeof ts == 'string') ts = Date.parse(ts) / 1000;

    return util.human_duration(new Date().getTime() / 1000 - ts) + ' ago';
  },


  duration: function (ts, precision) {
    return util.human_duration(parseInt(ts), precision)
  },


  size: function (x, precision) {return util.human_size(x, precision)}
}


module.exports = function () {
  for (var name in filters)
    Vue.filter(name, filters[name])
}
