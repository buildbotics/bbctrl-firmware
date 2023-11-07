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


module.exports = {
  template: '#settings-admin-template',
  props: ['config', 'state'],


  data() {
    return {
      enableKeyboard: true,
      autoCheckUpgrade: true,
      password: '',
      firmwareName: '',
    }
  },


  ready() {
    this.enableKeyboard = this.config.admin['virtual-keyboard-enabled']
    this.autoCheckUpgrade = this.config.admin['auto-check-upgrade']
  },


  methods: {
    backup() {
      document.getElementById('download-target').src = '/api/config/download'
    },


    async restore_config() {
      return this.$root.upload({
        accept: '.json',
        on_confirm: this.restore
      })
    },


    restore(file) {
      let fr = new FileReader()
      fr.onload = async e => {
        let config

        try {
          config = JSON.parse(e.target.result)
        } catch (ex) {
          return this.$root.error_dialog("Invalid config file")
        }

        let data = await this.$api.put('config/save', config)
        this.$dispatch('update')
        this.$root.success_dialog('Configuration restored.')
      }

      fr.readAsText(file)
    },


    async do_reset() {
      await this.$api.put('config/reset')
      this.$dispatch('update')
      return this.$root.success_dialog('Configuration reset.')
    },


    async reset() {
      let response = await this.$root.open_dialog({
        title: 'Reset to default configuration?',
        body: 'Non-network configuration changes will be lost.',
        buttons: 'Cancel OK'
      })

      if (response == 'ok') this.do_reset()
    },


    check() {this.$dispatch('check', true)},


    async upgrade() {
      this.password = ''

      if (await this.$refs.upgrade.open() == 'upgrade') {
        await this.$api.put('upgrade', {password: this.password})
        await this.$refs.upgrading.open()
      }
    },


    async upload() {
      this.password = ''

      try {
        await this.$root.upload({
          url: 'firmware/update',
          accept: '.bz2',
          form(file) {
            return {
              firmware: file,
              password: this.password
            }
          },

          async on_confirm(file) {
            this.firmwareName = file.name
            return await this.$refs.upload.open() == 'upload'
          },
        })

        await this.$refs.upgrading.open()

      } catch (e) {
        return this.error_dialog('Invalid password or bad firmware')
      }
    },


    upgrade_confirmed() {this.$refs.upload.close('upgrade')},
    upload_confirmed()  {this.$refs.upload.close('upload')},


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
