/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2022, Buildbotics LLC, All rights reserved.

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

let api  = require('./api')
let util = require('./util')


function fail_message(xhr, path) {
  if (xhr.status == 404) return '<tt>' + path + '</tt> not found'
  return 'Failed to load <tt>' + path + '</tt>'
}


function download_array(url) {
  let d = $.Deferred()

  api.download(url, 'arraybuffer')
    .done((data) => {d.resolve(new Float32Array(data))})
    .fail(d.reject)

  return d.promise()
}


class Program {
  constructor(path) {
    this.path = path
    this.filename = util.display_path(path)
    this.progress = 0
  }


  positions() {
    if (!this._positions)
      this._positions = download_array('positions/' + this.path)

    return this._positions
  }


  speeds() {
    if (!this._speeds) this._speeds = download_array('speeds/' + this.path)
    return this._speeds
  }


  view() {
    if (!this._view) {
      let d = $.Deferred()

      this.toolpath().done((toolpath) => {
        $.when(this.positions(), this.speeds())
          .done((positions, speeds) => {
            toolpath.positions = positions
            toolpath.speeds = speeds
            d.resolve(toolpath)

          }).fail(d.reject)

      }).fail(d.reject)

      this._view = d.promise()
    }

    return this._view
  }


  toolpath() {
    if (!this._toolpath) {
      let d = $.Deferred()
      let retry = 0
      let _load = () => {
        api.get('path/' + this.path)
          .done((toolpath) => {
            if (toolpath.progress == undefined) {
              this.progress = 1
              util.update_object(this, toolpath)
              d.resolve(toolpath)

            } else {
              this.progress = toolpath.progress
              _load() // Try again
            }

          }).fail((error, xhr) => {
            if (xhr.status == 404 || ++retry < 10)
              d.reject(fail_message(xhr, this.path), error)
          })
      }

      _load()
      this._toolpath = d.promise()
    }

    return this._toolpath
  }


  load() {
    if (!this._load) {
      let d = $.Deferred()

      api.download('fs/' + this.path)
        .done(d.resolve)
        .fail((error, xhr) => {d.reject(fail_message(xhr, this.path), error)})

      this._load = d.promise()
    }

    return this._load
  }
}


module.exports = Program
