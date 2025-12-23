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
        tool: {},  // FIX: Initialize tool config for reactivity
        version: '<loading>'
      },
      // FIX: Initialize state properties for Vue 1.x reactivity
      // Properties must exist before Vue processes the component
      state: {
        messages: [],
        service_due_count: 0,
        s: 0,           // Spindle speed - needed for popupMessagesHeader reactivity
        xx: '',         // Machine state
        line: 0,        // Current line
        v: 0,           // Velocity
        feed: 0,        // Feed rate
        speed: 0,       // Programmed speed
        tool: 0         // Current tool
      },
      crosshair: cookie.get_bool('crosshair', false),
      selected_program: new Program(this.$api, cookie.get('selected-path')),
      active_program: undefined,
      errorTimeout: 30,
      errorTimeoutStart: 0,
      errorMessage: '',
      checkedUpgrade: false,
      latestVersion: '',
      webGLSupported: util.webgl_supported(),
      // Track if we've initialized after connect
      initialized: false,
      // Alert dismissal state (session-based, resets on browser close)
      upgrade_dismissed: sessionStorage.getItem('upgrade_dismissed') === 'true',
      service_dismissed: sessionStorage.getItem('service_dismissed') === 'true',
      // Fullscreen state
      is_fullscreen: false
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
    'view-macros':    require('./view-macros'),
    'view-service':   require('./view-service'),
    'view-camera':    {template: '#view-camera-template'},
    'view-docs':      require('./view-docs')
  },


  watch: {
    crosshair() {cookie.set_bool('crosshair', this.crosshair)},


    // Watch for active_program changes from server
    // Only update the active_program object - don't clear selected_program here
    // (selected_program should persist through macro runs)
    'state.active_program'(path) {
      if (!path || path == '<mdi>') {
        this.active_program = undefined
      } else {
        this.active_program = new Program(this.$api, path)
      }
    },


    // Watch for e-stop state to clear programs
    'state.xx'(state) {
      if (state == 'ESTOPPED') {
        // Clear programs on e-stop for safety
        this.clear_selected_program()
      }
    },


    'state.first_file'(value) {
      // Only auto-select first file if we have no selection
      if (!this.selected_program.path && value) {
        this.select_path(value)
      }
    },


    // Reset upgrade dismissal when version changes
    latestVersion(newVersion, oldVersion) {
      if (oldVersion && newVersion !== oldVersion) {
        this.upgrade_dismissed = false
        sessionStorage.removeItem('upgrade_dismissed')
      }
    },


    // Reset service dismissal when due count changes (new items become due)
    'state.service_due_count'(newCount, oldCount) {
      if (oldCount !== undefined && newCount > oldCount) {
        this.service_dismissed = false
        sessionStorage.removeItem('service_dismissed')
      }
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
      
      // On initial connection, check if server has no active program
      // and clear our cached selection to sync with server state
      if (!this.initialized) {
        this.initialized = true
        if (!this.state.active_program) {
          this.clear_selected_program()
        }
      }
      
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
    is_local() {return location.host == 'localhost'},


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


    // Dynamic header for GCode messages modal showing spindle speed
    // Shows real-time spindle speed during M0 pause when tool is configured
     popupMessagesHeader() {
      let header = 'GCode Messages'

      // Show spindle speed when tool is configured (not Disabled)
      // NOTE: Access state.s unconditionally for Vue 1.x reactivity tracking
      let toolType = this.config.tool && this.config.tool['tool-type']
      let speed = parseFloat(this.state.s)

      if (toolType && toolType !== 'Disabled' && !isNaN(speed)) {
        header += ' - ' + Math.round(speed) + ' RPM'
      }

      return header
    },


    show_upgrade() {
      if (!this.latestVersion) return false
      return util.compare_versions(this.config.version, this.latestVersion) < 0
    },


    show_upgrade_alert() {
      return this.show_upgrade && !this.upgrade_dismissed
    },


    show_service_alert() {
      let count = this.state.service_due_count || 0
      return count > 0 && !this.service_dismissed
    },


    camera_available() {
      // Use state from backend, default to true if not yet received
      return this.state.camera_available !== false
    }
  },


  async ready() {
    this.$api.set_error_handler(this.error_dialog)

    if (this.is_local) this.$kbd.install()

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

    // Listen for fullscreen changes (e.g., user presses Escape)
    document.addEventListener('fullscreenchange', () => {
      this.is_fullscreen = !!document.fullscreenElement
    })
  },


  methods: {
    dismiss_upgrade() {
      this.upgrade_dismissed = true
      sessionStorage.setItem('upgrade_dismissed', 'true')
    },


    dismiss_service() {
      this.service_dismissed = true
      sessionStorage.setItem('service_dismissed', 'true')
    },


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

      let url  = 'https://buildbotics.com/bbctrl-2.0/latest.txt'
      let conf = {params: {hid: this.state.hid}, type: 'text'}
      if (!show_message) conf.error = () => {} // Ignore errors
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
        if (check == undefined || check) this.check_version()
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
            if (typeof this.hostname != 'undefined' && !this.is_local)
              location.hostname = this.hostname
            location.reload(true)
          }
        }

        util.update_object(this.state, e.data, false)
        this.$broadcast('update')
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


    // Clear selected program and cookie
    clear_selected_program() {
      cookie.set('selected-path', '')
      this.selected_program = new Program(this.$api, '')
      // Broadcast that program was cleared so views can update
      this.$broadcast('program-cleared')
    },


    // Handle re-selecting same path to ensure fresh file content
    select_path(path, force) {
      if (path && this.selected_program.path != path) {
        cookie.set('selected-path', path)
        this.selected_program = new Program(this.$api, path)
      } else if (path && force) {
        // Same path but force refresh - invalidate cached data
        this.selected_program.invalidate()
      }

      return this.selected_program
    },
    
    
    // Reload current program - creates new instance to force fresh fetch
    // Used after file upload to ensure new content is displayed
    reload_selected_program() {
      if (this.selected_program && this.selected_program.path) {
        let path = this.selected_program.path
        // Create new Program instance - guarantees fresh data fetch
        this.selected_program = new Program(this.$api, path)
        // Broadcast to trigger reload in view-control
        this.$broadcast('program-reloaded')
      }
    },


    edit(path) {
      this.select_path(path)
      location.hash = 'editor'
    },


    view(path) {
      this.select_path(path)
      location.hash = 'viewer'
    },


    toggle_fullscreen() {
      if (!document.fullscreenElement) {
        document.documentElement.requestFullscreen().then(() => {
          this.is_fullscreen = true
        }).catch(err => {
          console.warn('Fullscreen request failed:', err)
        })
      } else {
        document.exitFullscreen().then(() => {
          this.is_fullscreen = false
        }).catch(err => {
          console.warn('Exit fullscreen failed:', err)
        })
      }
    }
  }
})
