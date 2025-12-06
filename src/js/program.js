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


let util = require('./util')


class Program {
  constructor($api, path) {
    this.$api = $api
    this.path = path
    this.filename = util.display_path(path)
    this.progress = 0
    // Track when program was created/refreshed for cache busting
    this.timestamp = Date.now()
  }


  // Invalidate cached data so files are re-fetched from server
  invalidate() {
    this._load = null
    this._toolpath = null
    this._positions = null
    this._speeds = null
    this._view = null
    // New timestamp ensures cache-busted URLs are unique
    this.timestamp = Date.now()
  }


  // Append cache-busting timestamp to URL
  // This forces browser to fetch fresh data after invalidate()
  _cacheBust(url) {
    return url + '?_t=' + this.timestamp
  }


  async download_array(url) {
    let data = await this.$api.get(this._cacheBust(url), {type: 'arraybuffer'})
    return new Float32Array(data)
  }


  positions() {
    if (!this._positions)
      this._positions = this.download_array('positions/' + this.path)

    return this._positions
  }


  speeds() {
    if (!this._speeds) this._speeds = this.download_array('speeds/' + this.path)
    return this._speeds
  }


  async _load_view() {
    let toolpath = await this.toolpath()
    let [positions, speeds] =
        await Promise.all([this.positions(), this.speeds()])

    toolpath.positions = positions
    toolpath.speeds    = speeds
    return toolpath
  }


  view() {
    if (!this._view) this._view = this._load_view()
    return this._view
  }


  async _load_toolpath() {
    let toolpath = await this.$api.get(this._cacheBust('path/' + this.path))

    if (toolpath.progress == undefined) {
      this.progress = 1
      util.update_object(this, toolpath)
      return toolpath

    } else {
      this.progress = toolpath.progress
      return this._load_toolpath() // Try again
    }
  }


  toolpath() {
    if (!this._toolpath) this._toolpath = this._load_toolpath()
    return this._toolpath
  }


  load() {
    // Cache the load promise so multiple callers get the same result
    // Cache-busting via timestamp ensures fresh content after invalidate()
    if (!this._load) {
      this._load = this.$api.get(this._cacheBust('fs/' + this.path), {type: 'text'})
    }
    return this._load
  }
}


module.exports = Program
