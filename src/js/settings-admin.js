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


module.exports = {
  template: '#settings-admin-template',
  props: ['config', 'state'],


  data() {
    return {
      users:            [],
      access_control:   false,
      local_access:     true,
      name:             '',
      password:         '',
      generated:        '',
      user_admin:       false,
      add_error:        '',
      enableKeyboard: true,
      autoCheckUpgrade: true,
    }
  },


  computed: {
    admin()      {return this.$root.admin},
    real_users() {return this.users.filter(user => !user.builtin)},
    first_user() {return !this.real_users.length},
    last_user()  {return this.access_control && this.real_users.length == 1},


    // The controller refuses to send or accept passwords in plain text
    needs_https() {return !this.$root.is_secure && this.$root.https != 0}
  },


  watch: {
    '$root.admin'(admin) {if (admin) this.update_users()}
  },


  async ready() {
    this.enableKeyboard   = this.config.admin['virtual-keyboard-enabled']
    this.autoCheckUpgrade = this.config.admin['auto-check-upgrade']
    this.update_users()
  },


  methods: {
    async update_users() {
      if (this.admin) this.set_users(await this.$api.get('auth/users'))
    },


    // Every change to the users returns the new state
    set_users(data) {
      this.users          = data.users
      this.access_control = data.access_control
      this.local_access   = data.local_access
    },


    async generate() {
      this.generated = (await this.$api.get('auth/passphrase')).passphrase
      this.password  = this.generated
    },


    clear_new_user() {
      this.name       = ''
      this.password   = ''
      this.generated  = ''
      this.user_admin = false
      this.add_error  = ''
    },


    async add() {
      this.clear_new_user()
      let was_off = !this.access_control

      // Report failures in the dialog and reopen it so nothing is retyped
      let ok = false
      while (!ok) {
        if (await this.$refs.addUser.open() != 'add') return

        if (!this.name) this.add_error = 'User name required'
        else if (!this.password) this.add_error = 'Password required'

        else try {
          this.set_users(await this.$api.post(
            'auth/users/' + this.name,
            {password: this.password, admin: this.user_admin},
            {error: (p, xhr) => this.add_error = this.$api.message(p, xhr)}))
          ok = true

        } catch (e) {} // Reported in the dialog
      }

      this.clear_new_user() // Don't keep the password around
      await this.$root.check_login()

      // Say so, the change is immediate but nothing else looks different
      if (was_off && this.access_control)
        return this.$root.success_dialog(
          '<p>Access control is on.  Every browser must now log in over ' +
          'HTTPS.  This one stays logged in as <b>' + this.$root.user +
          '</b> until it is closed or the controller restarts.</p>')
    },


    async set_admin(user) {
      // The checkbox has already been toggled by the model
      if (!user.admin && user.name == this.$root.user) {
        let response = await this.$root.open_dialog({
          icon:    'exclamation-triangle',
          header:  'Remove your own admin access?',
          body:    '<p>You will lose access to this page immediately.  Log ' +
                   'in as <b>admin</b> with the system password to get it ' +
                   'back.</p>',
          width:   '400px',
          buttons: [{text: 'Cancel'}, {text: 'Remove', class: 'button-error'}]
        })

        if (response != 'remove') {
          user.admin = true // Restore the checkbox
          return
        }
      }

      try {
        this.set_users(await this.$api.put('auth/users/' + user.name,
                                           {admin: user.admin}))
      } catch (e) {
        await this.update_users() // Resynchronize the checkbox
      }

      if (user.name == this.$root.user) await this.$root.check_login()
    },


    async remove(user) {
      let msg = 'Remove user <b>' + user.name + '</b>?'

      if (this.last_user)
        msg += '  This is the last user so only <b>admin</b> will be able ' +
          'to log in.'
      else if (user.name == this.$root.user)
        msg += '  This is your own account and you will be logged out.'

      let response = await this.$root.open_dialog({
        icon:    'exclamation-triangle',
        header:  'Remove user',
        body:    '<p>' + msg + '</p>',
        width:   '400px',
        buttons: [{text: 'Cancel'}, {text: 'Remove', class: 'button-error'}]
      })

      if (response != 'remove') return

      this.set_users(await this.$api.delete('auth/users/' + user.name))
      await this.$root.check_login()
    },


    async set_access_control() {
      // The checkbox has already been toggled by the model
      if (!this.access_control) {
        let response = await this.$root.open_dialog({
          icon:    'exclamation-triangle',
          header:  'Disable access control?',
          body:    '<p>Anyone who can reach this controller over the ' +
                   'network will be able to run the machine and change its ' +
                   'configuration without logging in.</p>',
          width:   '400px',
          buttons: [{text: 'Cancel'}, {text: 'Disable', class: 'button-error'}]
        })

        if (response != 'disable') {
          this.access_control = true
          return
        }
      }

      try {
        this.set_users(await this.$api.put(
          'auth/access', {access_control: this.access_control}))
      } catch (e) {
        await this.update_users() // Resynchronize the checkbox
      }

      await this.$root.check_login()
    },


    async set_local_access() {
      // The checkbox has already been toggled by the model
      try {
        this.set_users(await this.$api.put(
          'auth/access', {local_access: this.local_access}))
      } catch (e) {
        await this.update_users() // Resynchronize the checkbox
      }

      await this.$root.check_login()
    },


    async _local_backup() {
      await this.$api.put('config/backup')
      return this.$root.success_dialog(
        'Configuration backup saved in "Home/configs/".')
    },


    backup() {
      if (this.$root.is_local) return this._local_backup()
      document.getElementById('download-target').src = '/api/config/download'
    },


    async _restore_config(config) {
      await this.$api.put('config/save', config)
      this.$dispatch('update')
      return this.$root.success_dialog('Configuration restored.')
    },


    async restore() {
      if (this.$root.is_local) {
        let path = await this.$root.file_dialog()
        if (!path) return

        let config = await this.$api.get('fs/' + path)
        return this._restore_config(config)

      } else
        return this.$root.upload({
          accept: '.json',

          on_confirm: async file => {
            return new Promise((resolve, reject) => {
              let fr = new FileReader()

              fr.onload = async e => {
                let config

                try {
                  config = JSON.parse(e.target.result)

                } catch (e) {
                  await this.$root.error_dialog("Invalid configuration.")
                  reject()
                }

                await this._restore_config(config)
              }

              fr.readAsText(file)
            })
          }
        })
    },


    async do_reset() {
      await this.$api.put('config/reset')
      this.$dispatch('update')
      return this.$root.success_dialog('Configuration reset.')
    },


    async reset() {
      let response = await this.$root.open_dialog({
        header: 'Reset to default configuration?',
        body: 'Non-network configuration changes will be lost.',
        buttons: 'Cancel Ok'
      })

      if (response == 'ok') this.do_reset()
    },


    check() {this.$root.check_version(true)},


    async upgrade() {
      if (await this.$refs.upgrade.open() == 'upgrade') {
        await this.$api.put('upgrade')
        await this.$refs.upgrading.open()
      }
    },


    async upload() {
      if (this.$root.is_local) {
        let path = await this.$root.file_dialog()
        if (!path) return
        await this.$api.put('firmware/update', {path})

      } else
        await this.$root.upload({
          url: 'firmware/update',
          accept: '.bz2',
          form: file => ({firmware: file}),
        })

      await this.$refs.upgrading.open()
    },


    change_auto_check_upgrade() {
      this.config.admin['auto-check-upgrade'] = this.autoCheckUpgrade
      this.$dispatch('config-changed')
    },


    change_enable_keyboard() {
      this.config.admin['virtual-keyboard-enabled'] = this.enableKeyboard
      this.$dispatch('config-changed')
      if (!this.enableKeyboard) this.$api.put('keyboard/hide')
    }
  }
}
