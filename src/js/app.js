/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2026, Buildbotics LLC, All rights reserved.

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
      access_control: false,
      https: 0, // The controller's HTTPS port, zero if not running
      admin: false,
      admin_user: false,
      user: '',
      login_user: 'admin',
      login_error: '',
      password_user: '',
      password_current: '',
      password_error: '',
      password: '',
      password2: '',
      generated: '',
      logging_in: false,
      session_expired: false,
      admin_timer: 0,
      currentView: 'loading',
      template: require('../resources/config-template.json'),
      config: {
        settings: {units: 'METRIC'},
        motors:   [{}, {}, {}, {}],
        tool:     {},
        version:  '<loading>'
      },
      state: {
        messages: [],
        s:        0,
        xx:       '',
        line:     0,
        v:        0,
        feed:     0,
        speed:    0,
        tool:     0,
      },
      crosshair:         cookie.get_bool('crosshair', false),
      selected_program:  new Program(this.$api, cookie.get('selected-path')),
      active_program:    undefined,
      errorTimeout:      30,
      errorTimeoutStart: 0,
      errorMessage:      '',
      checkedUpgrade:    false,
      latestVersion:     '',
      webGLSupported:    util.webgl_supported(),
      initialized:       false,
    }
  },


  components: {
    'estop':          {template: '#estop-template'},
    'view-loading':   {template: '<h1>Loading...</h1>'},
    'view-control':   require('./view-control'),
    'view-macros':    require('./view-macros'),
    'view-viewer':    require('./view-viewer'),
    'view-editor':    require('./view-editor'),
    'view-settings':  require('./view-settings'),
    'view-files':     require('./view-files'),
    'view-camera':    {template: '#view-camera-template'},
    'view-docs':      require('./view-docs')
  },


  watch: {
    crosshair() {cookie.set_bool('crosshair', this.crosshair)},


    login_required(required) {
      if (!required) return
      this.disconnect() // Don't retry the websocket while logged out
      this.login()
    },


    'state.active_program'(path) {
      if (!path || path == '<mdi>') this.active_program = undefined
      else this.active_program = new Program(this.$api, path)
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
    is_local() {return location.host == 'localhost'},
    login_required() {return !this.authorized},


    // Matches the controller's idea of a secure connection
    is_secure() {
      return location.protocol == 'https:' ||
        ['localhost', '127.0.0.1', '[::1]'].includes(location.hostname)
    },


    // The built in admin's password is the system password
    system_password() {return this.password_user == 'admin'},


    // The user has admin access but it timed out, logging in restores it
    admin_expired() {return this.admin_user && !this.admin},


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
    this.$api.set_unauthorized_handler(this.check_login)

    if (this.is_local) this.$kbd.install()

    window.addEventListener('hashchange', this.parse_hash)

    await this.check_login()
    if (this.login_required) await this.login()
    else this.connect()

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
  },


  methods: {
    async check_login() {
      let login

      // Ignore errors, the controller may be unreachable
      try {
        login = await this.$api.get('auth/login', {error: () => {}})
      } catch (e) {return}

      let was_logged_in = this.user

      this.user           = login.user || ''
      this.admin          = login.admin
      this.admin_user     = login.admin_user
      this.authorized     = login.authorized
      this.access_control = login.access_control
      this.https          = login.https || 0

      // Explain why the login dialog appeared
      if (was_logged_in && !this.user) this.session_expired = true

      // Admin access expires on its own, drop it from the interface with it
      clearTimeout(this.admin_timer)
      if (login.admin)
        this.admin_timer =
          setTimeout(this.check_login, (login.admin_left + 1) * 1000)

      // Don't change the name while the user is typing it
      if (this.user && !this.logging_in) this.login_user = this.user
    },


    // The controller refuses to accept a password in plain text
    async require_https() {
      if (this.is_secure || !this.https) return false

      let response = await this.open_dialog({
        icon:    'lock',
        header:  'HTTPS required',
        body:    '<p>Passwords may only be sent over a secure connection.  ' +
                 'The controller creates its own certificate, so the browser ' +
                 'cannot vouch for it and will ask you to continue the first ' +
                 'time.</p>',
        width:   '400px',
        buttons: [{text: 'Cancel'},
                  {text: 'Use HTTPS', class: 'pure-button-primary'}]
      })

      if (response == 'use https') this.goto_https()
      return true
    },


    async login(user) {
      if (await this.require_https()) return
      if (this.logging_in) return // Already showing the login dialog

      if (user) this.login_user = user
      this.logging_in = true

      try {
        do {
          this.password = ''
          if (await this.$refs.loginDialog.open() != 'login') break

          try {
            // Report the failure in the dialog, an error dialog would open
            // behind it
            await this.$api.put('auth/login',
                                {user: this.login_user,
                                 password: this.password},
                                {error: (path, xhr) => this.login_error =
                                 this.$api.message(path, xhr)})
          } catch (e) {continue} // Reported in the dialog

          this.login_error = ''
          await this.check_login()
        } while (this.login_required)

      } finally {
        this.password        = ''
        this.login_error     = ''
        this.logging_in      = false
        this.session_expired = false
      }

      if (!this.authorized) return

      // Clear errors reported before the login, such as a mistyped password
      this.$refs.dialog.close()
      if (this.status != 'connected') this.connect()
    },


    async generate_passphrase() {
      this.generated = (await this.$api.get('auth/passphrase')).passphrase
      this.password  = this.generated
      this.password2 = this.generated
    },


    clear_password() {
      this.password_current = ''
      this.password_error   = ''
      this.password         = ''
      this.password2        = ''
      this.generated        = ''
    },


    // Changes the password of the logged in user, which requires confirming
    // the current one, or of any user for an admin
    async change_password(name) {
      if (await this.require_https()) return

      this.password_user = name
      this.clear_password()

      let own = name == this.user
      let ok  = false

      while (!ok) {
        if (await this.$refs.passwordDialog.open({header:
          'Change password for "' + name + '"'}) != 'set') return

        if (own && !this.password_current)
          this.password_error = 'Your current password is required'

        else if (this.password != this.password2)
          this.password_error = 'Passwords do not match'

        else try {
          let data = {password: this.password}
          if (own) data.current = this.password_current

          // Report failures in the dialog, an error dialog opens behind it
          await this.$api.put(
            own ? 'auth/password' : 'auth/users/' + name, data,
            {error: (p, xhr) =>
             this.password_error = this.$api.message(p, xhr)})
          ok = true

        } catch (e) {} // Reported in the dialog
      }

      this.clear_password()

      return this.success_dialog('Password changed for "' + name + '".')
    },


    // Reloads the page over HTTPS so the certificate can be accepted
    goto_https() {
      let port = this.https == 443 ? '' : ':' + this.https
      location.href = 'https://' + location.hostname + port +
        location.pathname + location.hash
    },


    async logout() {
      this.user = '' // Logging out is not a session expiry
      await this.$api.delete('auth/login')
      await this.check_login() // Gates the interface if a login is required
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


    disconnect() {if (this.sock) this.sock.close()},


    connect() {
      if (this.sock) return this.sock.connect()

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
        this.check_login() // The session may have expired
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
