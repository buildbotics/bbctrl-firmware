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


class API {
  constructor(base = '/api/') {
    this.base = base
    this.error_handler = console.error
  }


  set_error_handler(handler) {
    this.error_handler = handler
  }


  error(path, xhr) {
    let msg

    if (xhr.responseType == 'json' && xhr.response && xhr.response.message)
      msg = xhr.response.message

    else if (xhr.statusText) msg = xhr.statusText

    if (!msg) msg = 'API Error: ' + path

    this.error_handler(msg)
  }


  api(method, path, data, conf = {}) {
    return new Promise((resolve, reject) => {
      let xhr = new XMLHttpRequest()

      xhr.onerror = e => {
        if (conf.error) conf.error(path, xhr)
        else this.error(path, xhr)

        reject({path, xhr})
      }

      xhr.onload = () => {
        let status = xhr.status

        if (200 <= status && status < 300) resolve(xhr.response)
        else xhr.onerror()
      }

      // Progress
      // TODO What about download progress?
      if (conf.progress) {
        xhr.upload.addEventListener('progress', e => {
          if (e.lengthComputable)
            conf.progress(e.loaded / e.total, false, xhr)
        })

        xhr.upload.addEventListener('loadend', e => {
          conf.progress(e.loaded / e.total, true, xhr)
        })
      }

      // URL
      if (!path.includes('://')) path = this.base + path
      let url = new URL(path, location.href)

      // Params
      if (conf.params) url.search = new URLSearchParams(conf.params).toString()

      // Open
      xhr.open(method, url)

      // JSON data
      if (data && !(data instanceof FormData)) {
        xhr.setRequestHeader('Content-Type',
                             'application/json; charset=utf-8')
        data = JSON.stringify(data)
      }

      // Response type
      if (method == 'GET' || conf.type)
        xhr.responseType = conf.type || 'json'

      if (xhr.responseType == 'json')
        xhr.setRequestHeader('Accept', 'application/json; charset=utf-8')

      if (xhr.responseType == 'text')
        xhr.setRequestHeader('Content-Type', 'text/plain')

      // Cache
      if (conf.cache === false)
        xhr.setRequestHeader('Cache-Control', 'no-cache, no-store, max-age=0')

      // Send it
      xhr.send(data)
    })
  }


  async get(path, conf) {return this.api('GET', path, undefined, conf)}
  async put(path, data, conf) {return this.api('PUT', path, data, conf)}
  async post(path, data, conf) {return this.api('POST', path, data, conf)}
  async delete(path, conf) {return this.api('DELETE', path, undefined, conf)}
}


module.exports = API
