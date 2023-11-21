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


function get_value(value, file, defaultValue) {
  if (typeof value == 'function') value = value(file)
  if (value == undefined) value = defaultValue
  return value
}


module.exports = {
  template: '#upload-dialog-template',


  data() {
    return {
      config: {},
      progress: 0,
      msg: ''
    }
  },


  methods: {
    async upload(config)  {
      this.canceled = false
      this.config   = config

      // Reset <form> so the browser does not cache the file when the name is
      // same but the contents have changed.
      this.$els.form.reset()

      let input = this.$els.input
      if (config.multiple) input.setAttribute('multiple', '')
      else input.removeAttribute('multiple')
      input.setAttribute('accept', get_value(config.accept, undefined, ''))
      input.click()

      return new Promise((resolve, reject) => {
        this.resolve = resolve
        this.reject  = reject
      })
    },


    cancel() {
      this.canceled = true
      if (this.xhr) this.xhr.abort()
    },


    async _upload_file(file) {
      let _progress = (fraction, done, xhr) => {
        this.xhr = xhr
        this.progress = (fraction * 100).toFixed(1)
      }

      this.progress = 0
      this.$refs.dialog.open()

      this.msg = get_value(this.config.msg,  file, file.name)
      let url  = get_value(this.config.url,  file)
      let form = get_value(this.config.form, file, {file: file})

      let fd = new FormData()
      for (const key in form)
        fd.append(key, get_value(form[key], file))

      return this.$api.put(url, fd, {progress: _progress})
    },


    async _upload(e)  {
      try {
        let files   = e.target.files || e.dataTransfer.files
        let confirm = this.config.on_confirm ?
            this.config.on_confirm : async () => true

        for (let i = 0; i < files.length && !this.canceled; i++)
          if (await confirm(files[i]))
            await this._upload_file(files[i])

      } finally {this.$refs.dialog.close()}
    }
  }
}
