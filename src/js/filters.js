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


let util = require('./util')


let filters = {
  number(value) {
    if (isNaN(value)) return 'NaN'
    return value.toLocaleString()
  },


  percent(value, precision) {
    if (typeof value == 'undefined') return ''
    if (typeof precision == 'undefined') precision = 2
    return (value * 100.0).toFixed(precision) + '%'
  },



  non_zero_percent(value, precision) {
    if (!value) return ''
    if (typeof precision == 'undefined') precision = 2
    return (value * 100.0).toFixed(precision) + '%'
  },


  fixed(value, precision) {
    if (typeof value == 'undefined') return '0'
    return parseFloat(value).toFixed(precision)
  },


  upper(value) {
    if (typeof value == 'undefined') return ''
    return value.toUpperCase()
  },


  time(value, precision) {
    if (isNaN(value)) return ''
    if (isNaN(precision)) precision = 0

    let MIN = 60
    let HR  = MIN * 60
    let DAY = HR * 24
    let parts = []

    if (DAY <= value) {
      parts.push(Math.floor(value / DAY))
      value %= DAY
    }

    if (HR <= value) {
      parts.push(Math.floor(value / HR))
      value %= HR
    }

    if (MIN <= value) {
      parts.push(Math.floor(value / MIN))
      value %= MIN

    } else parts.push(0)

    parts.push(value)

    for (let i = 0; i < parts.length; i++) {
      parts[i] = parts[i].toFixed(i == parts.length - 1 ? precision : 0)
      if (i && parts[i] < 10) parts[i] = '0' + parts[i]
    }

    return parts.join(':')
  },


  ago(ts) {
    if (typeof ts == 'string') ts = Date.parse(ts) / 1000

    return util.human_duration(new Date().getTime() / 1000 - ts) + ' ago'
  },


  duration(ts, precision) {
    return util.human_duration(parseInt(ts), precision)
  },


  size(x, precision) {return util.human_size(x, precision)}
}


module.exports = () => {
  for (let name in filters)
    Vue.filter(name, filters[name])
}
