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


let cookie  = require('./cookie')
let Sock    = require('./sock')
let util    = require('./util')
let Program = require('./program')


module.exports = new Vue({
  el: 'body',

  data() {
    return {
      status: 'connecting',
      authorized: false,
      password: '',
      currentView: 'loading',
      template: require('../resources/config-template.json'),
      config: {
        settings: {units: 'METRIC'},
        motors: [{}, {}, {}, {}],
        version: '<loading>'
      },
      state: {messages: []},
      crosshair: cookie.get_bool('crosshair', false),
      selected_program: new Program(this.$api, cookie.get('selected-path')),
      active_program: undefined,
      errorTimeout: 30,
      errorTimeoutStart: 0,
      errorMessage: '',
      checkedUpgrade: false,
      latestVersion: '',
      webGLSupported: util.webgl_supported()
    }
  },


  components: {
    'estop':          {template: '#estop-template'},
    'view-loading':   {template: '<h1>Loading...</h1>'},
    'view-control':   require('./view-control'),
    'view-viewer':    require('./view-viewer'),
    'view-editor':    require('./view-editor'),
    'view-settings':  require('./view-settings'),
    'view-files':     require('./view-files'),
    'view-camera':    {template: '#view-camera-template'},
    'view-docs':      require('./view-docs')
  },


  watch: {
    crosshair() {cookie.set_bool('crosshair', this.crosshair)},


    'state.active_program'() {
      let path = this.state.active_program
      if (!path || path == '<mdi>') this.active_program = undefined
      else new Program(this.$api, path)
    },


    'state.first_file'(value) {
      if (!this.selected_program.path) this.select_path(value)
    }
  },


  events: {
    route(path) {
      let oldView = this.currentView

      if (typeof this.$options.components['view-' + path[0]] == 'undefined')
        return location.hash = 'control'
      else this.currentView = path[0]

      if (oldView != this.currentView) $('#main').scrollTop(0)
    },


    'hostname-changed'(hostname) {this.hostname = hostname},


    send(msg) {
      if (this.status == 'connected') {
        console.debug('>', msg)
        this.sock.send(msg)
      }
    },


    async connected() {
      await this.update()
      this.parse_hash()
    },


    async update() {
      await this.update()
      this.parse_hash()
    },


    error(msg) {
      // Honor user error blocking
      if (Date.now() - this.errorTimeoutStart < this.errorTimeout * 1000)
        return

      // Wait at least 1 sec to pop up repeated errors
      if (1 < msg.repeat && Date.now() - msg.ts < 1000) return

      // Popup error dialog
      this.errorMessage = msg.msg
      this.$refs.errorMessage.open()
    }
  },


  computed: {
    popupMessages() {
      let msgs = []

      for (let i = 0; i < this.state.messages.length; i++) {
        let text = this.state.messages[i].text
        msgs = msgs.concat(text.split('#'))
      }

      if (msgs.length) this.$refs.popupMessages.open()
      else this.$refs.popupMessages.close()

      return msgs
    },


    show_upgrade() {
      if (!this.latestVersion) return false
      return util.compare_versions(this.config.version, this.latestVersion) < 0
    }
  },


  async ready() {
    this.$api.set_error_handler(this.error_dialog)

    window.addEventListener('hashchange', this.parse_hash)
    this.connect()

    if (!this.webGLSupported) {
      let msg = 'Your browser does not have hardware support for 3D ' +
          'graphics.  3D viewer disabled.'

      console.warn(msg)

      if (!cookie.get_bool('webgl-warning', false)) {
        await this.open_dialog({
          header: 'WebGL Warning',
          body: msg,
          buttons: 'ok'
        })

        cookie.set_bool('webgl-warning', true)
      }
    }

    this.check_login()
  },


  methods: {
    async check_login() {
      this.authorized = await this.$api.get('auth/login')
    },


    async login() {
      this.password = ''
      let response = await this.$refs.loginDialog.open()

      if (response == 'login' && this.password) {
        await this.$api.put('auth/login', {password: this.password})
        this.authorized = true
      }
    },


    async logout() {
      await this.$api.delete('auth/login')
      this.authorized = false
    },


    async check_version(show_message) {
      this.latestVersion = ''

      let url  = 'https://buildbotics.com/bbctrl/latest.txt'
      let conf = {data: {hid: this.state.hid}, type: 'text'}
      this.latestVersion = await this.$api.get(url, conf)

      if (!show_message) return
      let cmp = util.compare_versions(this.config.version, this.latestVersion)

      let msg
      if (cmp == 0) msg = 'You have the latest official firmware.'
      else {
        msg = 'Your firmware is ' + (cmp < 0 ? 'older': 'newer') +
          ' than the latest official firmware release, version ' +
          this.latestVersion + '.'

        if (cmp < 0) msg += ' Please upgrade.'
      }

      return this.open_dialog({
        icon: cmp ? (cmp < 0 ? 'chevron-left' : 'chevron-right') : 'check',
        header: 'Firmware check',
        body: msg
      })
    },


    metric() {return this.config.settings.units != 'IMPERIAL'},


    block_error_dialog() {
      this.errorTimeoutStart = Date.now()
      this.$refs.errorMessage.close()
    },


    async estop() {
      if (this.state.xx == 'ESTOPPED') return this.$api.put('clear')
      return this.$api.put('estop')
    },


    async update() {
      let config = await this.$api.get('config/load')

      util.update_object(this.config, config, true)

      if (!this.checkedUpgrade) {
        this.checkedUpgrade = true

        let check = this.config.admin['auto-check-upgrade']
        if (check == undefined || check) await this.check_version()
      }
    },


    connect() {
      this.sock = new Sock('//' + window.location.host + '/sockjs')

      this.sock.onmessage = (e) => {
        if (typeof e.data != 'object') return

        if ('log' in e.data) {
          this.$broadcast('log', e.data.log)
          delete e.data.log
        }

        // Check for session ID change on controller
        if ('sid' in e.data) {
          if (typeof this.sid == 'undefined') this.sid = e.data.sid

          else if (this.sid != e.data.sid) {
            if (typeof this.hostname != 'undefined' &&
                String(location.hostname) != 'localhost')
              location.hostname = this.hostname
            location.reload(true)
          }
        }

        util.update_object(this.state, e.data, false)
        this.$broadcast('update')

        // Disable WebGL on Pi 3 for local head
        if (this.webGLSupported) {
          let model = this.state.rpi_model
          if (location.hostname == 'localhost' && model != undefined &&
              model.indexOf('Pi 3') != -1)
            this.webGLSupported = false
        }
      }

      this.sock.onopen = (e) => {
        this.status = 'connected'
        this.$emit(this.status)
        this.$broadcast(this.status)
      }

      this.sock.onclose = (e) => {
        this.status = 'disconnected'
        this.$emit(this.status)
        this.$broadcast(this.status)
      }
    },


    route(path) {
      let cancel = false
      let cb = () => {cancel = true}

      this.$emit('route-changing', path, cb)
      this.$broadcast('route-changing', path, cb)

      if (cancel) return history.back()
      this.$emit('route', path)
      this.$broadcast('route', path)
    },


    replace_route(hash) {
      history.replaceState(null, '', '#' + hash)
      this.parse_hash()
    },


    parse_hash() {
      let hash = location.hash.substr(1)
      if (!hash.trim().length) return location.hash = 'control'
      this.route(hash.split(':'))
    },


    async close_messages(action) {
      if (action == 'stop')     await this.$api.put('stop')
      if (action == 'continue') await this.$api.put('unpause')

      // Acknowledge messages
      if (this.state.messages.length) {
        let id = this.state.messages.slice(-1)[0].id
        await this.$api.put('message/' + id + '/ack')
      }
    },


    async file_dialog(config) {return this.$refs.fileDialog.open(config)},
    async upload(config)      {return this.$refs.uploader.upload(config)},
    async open_dialog(config) {return this.$refs.dialog.open(config)},
    async error_dialog(msg)   {return this.$refs.dialog.error(msg)},
    async warning_dialog(msg) {return this.$refs.dialog.warning(msg)},
    async success_dialog(msg) {return this.$refs.dialog.success(msg)},


    async run(path) {
      if (this.state.xx != 'READY') return
      await this.$api.put('start/' + path)
      location.hash = 'control'
    },


    select_path(path) {
      if (path && this.selected_program.path != path) {
        cookie.set('selected-path', path)
        this.selected_program = new Program(this.$api, path)
      }

      return this.selected_program
    },


    edit(path) {
      this.select_path(path)
      location.hash = 'editor'
    },


    view(path) {
      this.select_path(path)
      location.hash = 'viewer'
    }
  }
})
