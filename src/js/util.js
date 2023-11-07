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



module.exports = {
  SEC_PER_YEAR:  365 * 24 * 60 * 60,
  SEC_PER_MONTH: 30 * 24 * 60 * 60,
  SEC_PER_WEEK:  7 * 24 * 60 * 60,
  SEC_PER_DAY:   24 * 60 * 60,
  SEC_PER_HOUR:  60 * 60,
  SEC_PER_MIN:   60,
  uuid_chars:
  'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+',


  duration(x, name, precision) {
    x = x.toFixed(precision == undefined ? 0 : precision)
    return x + ' ' + name + (x == 1 ? '' : 's')
  },


  human_duration(x, precision) {
    if (this.SEC_PER_YEAR <= x)
      return this.duration(x / this.SEC_PER_YEAR,  'year',  precision)
    if (this.SEC_PER_MONTH <= x)
      return this.duration(x / this.SEC_PER_MONTH, 'month', precision)
    if (this.SEC_PER_WEEK <= x)
      return this.duration(x / this.SEC_PER_WEEK,  'week',  precision)
    if (this.SEC_PER_DAY <= x)
      return this.duration(x / this.SEC_PER_DAY,   'day',   precision)
    if (this.SEC_PER_HOUR <= x)
      return this.duration(x / this.SEC_PER_HOUR,  'hour',  precision)
    if (this.SEC_PER_MIN <= x)
      return this.duration(x / this.SEC_PER_MIN,   'min',   precision)
    return this.duration(x, 'sec', precision)
  },


  human_size(x, precision) {
    if (typeof precision == 'undefined') precision = 1

    if (1e12 <= x) return (x / 1e12).toFixed(precision) + 'T'
    if (1e9  <= x) return (x / 1e9 ).toFixed(precision) + 'B'
    if (1e6  <= x) return (x / 1e6 ).toFixed(precision) + 'M'
    if (1e3  <= x) return (x / 1e3 ).toFixed(precision) + 'K'
    return x
  },


  unix_path(path) {
    if (/Win/i.test(navigator.platform)) return path.replace('\\', '/')
    return path
  },


  dirname(path) {
    let sep = path.lastIndexOf('/')
    return sep == -1 ? '.' : (sep == 0 ? '/' : path.substr(0, sep))
  },


  basename(path) {return path.substr(path.lastIndexOf('/') + 1)},


  join_path(a, b) {
    if (!a) return b
    return a[a.length - 1] == '/' ? a + b : (a + '/' + b)
  },


  display_path(path) {
    if (path == undefined) return path
    return path.startsWith('Home/') ? path.substr(5) : path
  },


  uuid(length) {
    if (typeof length == 'undefined') length = 52

    let s = ''
    for (let i = 0; i < length; i++)
      s += this.uuid_chars[Math.floor(Math.random() * this.uuid_chars.length)]

    return s
  },


  get_highlight_mode(path) {
    if (path.endsWith('.tpl') || path.endsWith('.js') ||
        path.endsWith('.json'))
      return 'javascript'

    return 'gcode'
  },


  webgl_supported() {
    try {
      return new THREE.WebGLRenderer({failIfMajorPerformanceCaveat: true})
    } catch (e) {return false}
  },


  compare_versions(a, b) {
    let reStripTrailingZeros = /(\.0+)+$/
    let segsA = a.trim().replace(reStripTrailingZeros, '').split('.')
    let segsB = b.trim().replace(reStripTrailingZeros, '').split('.')
    let l = Math.min(segsA.length, segsB.length)

    for (let i = 0; i < l; i++) {
      let diff = parseInt(segsA[i], 10) - parseInt(segsB[i], 10)
      if (diff) return diff
    }

    return segsA.length - segsB.length
  },


  is_object(o) {return o !== null && typeof o == 'object'},
  is_array(o) {return Array.isArray(o)},


  update_array(dst, src) {
    while (dst.length) dst.pop()
    for (let i = 0; i < src.length; i++)
      Vue.set(dst, i, src[i])
  },


  update_object(dst, src, remove) {
    let props, index, key, value

    if (remove) {
      props = Object.getOwnPropertyNames(dst)

      for (index in props) {
        key = props[index]
        if (!src.hasOwnProperty(key))
          Vue.delete(dst, key)
      }
    }

    props = Object.getOwnPropertyNames(src)
    for (index in props) {
      key = props[index]
      value = src[key]

      if (this.is_array(value) && dst.hasOwnProperty(key) &&
          this.is_array(dst[key])) this.update_array(dst[key], value)

      else if (this.is_object(value) && dst.hasOwnProperty(key) &&
               this.is_object(dst[key]))
        this.update_object(dst[key], value, remove)

      else Vue.set(dst, key, value)
    }
  }
}
