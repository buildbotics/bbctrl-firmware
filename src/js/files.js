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


function order_files(a, b) {
  if (a.dir != b.dir) return a.dir ? -1 : 1
  return a.name.localeCompare(b.name)
}


function valid_filename(name) {
  return name.length && name[0] != '.' && name.indexOf('/') == -1
}


module.exports = {
  template: '#files-template',
  props: ['mode', 'locations'],


  data() {
    return {
      fs: {},
      selected: -1,
      filename: '',
      folder: '',
    }
  },


  watch: {
    selected() {
      let path
      let is_dir = false

      if (0 <= this.selected && this.selected <= this.files.length) {
        let file = this.files[this.selected]
        if (file.dir) is_dir = true
        path = this.file_path(file)
      }

      if (this.mode != 'save') this.$emit('selected', path, is_dir)
      if (path && !is_dir) this.filename = util.basename(path)
    },


    filename() {
      if (this.mode != 'save') return
      let path
      if (this.filename_valid)
        path = util.join_path(this.fs.path, this.filename)
      this.$emit('selected', path, false)
    },


    locations() {
      if (this.locations.indexOf(this.location) == -1)
        this.load('')
    }
  },


  computed: {
    files() {
      if (typeof this.fs.files == 'undefined') return []
      return this.fs.files.sort(order_files)
    },


    location() {
      if (typeof this.fs.path != 'undefined') {
        let paths = this.fs.path.split('/').filter(s => s)
        if (paths.length) return paths[0]
      }

      return 'Home'
    },


    paths() {
      if (typeof this.fs.path == 'undefined') return []

      let paths = this.fs.path.split('/').filter(s => s)
      if (paths.length) paths.shift() // Remove location
      paths.unshift('/')

      return paths
    },


    folder_valid() {
      let file = this.find_file(this.folder)
      return file == undefined && valid_filename(this.folder)
    },


    filename_valid() {
      let file = this.find_file(this.filename)
      return (file == undefined || !file.dir) && valid_filename(this.filename)
    }
  },


  ready() {this.load('')},


  methods: {
    location_title(name) {
      if (name == 'Home')
        return 'Select files already on the controller.'
      return 'Select files from a USB drive.'
    },


    filename_changed() {
      if (this.selected != -1 &&
          this.filename != this.files[this.selected].name)
        this.selected = -1
    },


    find_file(name) {
      for (let i = 0; i < this.files.length; i++)
        if (this.files[i].name == name) return this.files[i]
      return undefined
    },


    has_file(name)  {return this.find_file(name) != undefined},
    file_path(file) {return util.join_path(this.fs.path, file.name)},
    file_url(file)  {return '/api/fs' + this.file_path(file)},
    select(index)   {this.selected = index},


    async eject(location) {return this.$api.put('usb/eject/' + location)},


    async open(path) {
      this.filename = ''
      await this.load(path)
    },


    async load(path) {
      let data = await this.$api.get('fs/' + path)
      this.fs = data
      this.selected = -1
    },


    async reload() {return this.load(this.fs.path || '')},


    path_at(index) {
      return '/' + this.paths.slice(1, index + 1).join('/')
    },


    path_title(index) {
      if (index == this.paths.length - 1) return ''
      return 'Go to folder ' + this.path_at(index)
    },


    load_path(index) {
      this.load(this.location + this.path_at(index))
    },


    async new_folder() {
      this.folder = ''
      let response = await this.$refs.newFolder.open()

      if (response == 'create') {
        let path = this.fs.path + '/' + this.folder
        await this.$api.put('fs/' + path)
        this.open(path)
      }
    },


    create_folder() {
      if (this.folder_valid) this.$refs.newFolder.close('create')
    },


    activate(file) {
      if (file.dir) this.load(this.fs.path + '/' + file.name)
      else this.$emit('activate', this.file_path(file))
    },


    async delete(file) {
      let response = await this.$root.open_dialog({
        header: 'Delete ' + (file.dir ? 'directory' : 'file') + '?',
        body: 'Are you sure you want to delete <tt>' + file.name +
          (file.dir ? '</tt> and all the files under it?' : '</tt>?'),
        buttons: 'Cancel Ok'
      })

      if (response == 'ok') {
        await this.$api.delete('fs/' + this.fs.path + '/' + file.name)
        this.reload()
      }
    },


    activate_file(filename) {
      for (let i = 0; i < this.files.length; i++)
        if (this.files[i].name == filename)
          this.activate(this.files[i])
    },


    async confirm_upload(file) {
      file.filename = util.basename(util.unix_path(file.name))

      let other = this.find_file(file.filename)
      if (!other) return true

      if (other.dir) {
        await this.$root.open_dialog({
          header: 'Cannot overwrite',
          body:   'Cannot overwrite directory ' + file.filename + '.',
        })

        return false
      }

      let response = await this.$root.open_dialog({
        header:  'Overwrite file?',
        body:    'Are you sure you want to overwrite ' + file.filename + '?',
        buttons: 'Cancel Ok'
      })

      return response == 'ok'
    },


    // FIX #5: Invalidate selected program cache after upload
    async upload()  {
      let filename
      let uploadedPath

      await this.$root.upload({
        multiple: this.mode != 'open',
        url: file => {
          filename = file.filename
          uploadedPath = this.fs.path + '/' + filename
          return 'fs/' + uploadedPath
        },
        on_confirm: this.confirm_upload,
      })

      await this.reload()
      
      // FIX #5: If we uploaded a file that matches the currently selected program,
      // invalidate the cache so fresh content is loaded
      if (uploadedPath && this.$root.selected_program) {
        let selectedPath = this.$root.selected_program.path
        if (selectedPath && selectedPath == uploadedPath) {
          this.$root.refresh_selected_program()
        }
      }
      
      if (this.mode == 'open') this.activate_file(filename)
    }
  }
}
