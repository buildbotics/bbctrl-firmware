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
      password:        '',
      password2:       '',
      enableKeyboard: true,
      autoCheckUpgrade: true,
    }
  },


  computed: {
    authorized() {return this.$root.authorized}
  },


  async ready() {
    this.enableKeyboard   = this.config.admin['virtual-keyboard-enabled']
    this.autoCheckUpgrade = this.config.admin['auto-check-upgrade']
  },


  methods: {
    async set_password() {
      if (this.password != this.password2)
        return this.$root.error_dialog('Passwords to not match')

      if (this.password.length < 6)
        return this.$root.error_dialog('Password too short')

      await this.$api.put('auth/password', {password: this.password})
      this.$root.success_dialog('Password Set')
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
