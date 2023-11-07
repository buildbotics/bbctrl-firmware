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
  template: '#settings-network-template',
  props: ['config', 'state'],


  components: {
    'wifi-connect': require('./wifi-connect'),
    'wifi-hotspot': require('./wifi-hotspot')
  },


  data() {
    return {
      hostnameSet:     false,
      redirectTimeout: 0,
      hostname:        '',
      username:        '',
      current:         '',
      password:        '',
      password2:       '',
      wifi_country:    'Unknown',
      wifi_pass:       '',
    }
  },


  async ready() {
    this.hostname = await this.$api.get('hostname')
    this.username = await this.$api.get('remote/username')
  },


  methods: {
    redirect(hostname) {
      if (0 < this.redirectTimeout) {
        this.redirectTimeout -= 1
        setTimeout(() => this.redirect(hostname), 1000)

      } else location.hostname = hostname
    },


    async set_hostname() {
      await this.$api.put('hostname', {hostname: this.hostname})
      this.redirectTimeout = 45
      this.hostnameSet = true

      await this.$api.put('reboot')

      if (String(location.hostname) == 'localhost') return

      let hostname = this.hostname
      if (String(location.hostname).endsWith('.local'))
        hostname += '.local'
      this.$dispatch('hostname-changed', hostname)
      this.redirect(hostname)
    },


    async set_username() {
      this.$api.put('remote/username', {username: this.username})
      this.$root.success_dialog('User name Set')
    },


    async set_password() {
      if (this.password != this.password2)
        return this.$root.error_dialog('Passwords to not match')

      if (this.password.length < 6)
        return this.$root.error_dialog('Password too short')

      await this.$api.put('remote/password', {
        current: this.current,
        password: this.password
      })

      this.$root.success_dialog('Password Set')
    },


    async config_wifi() {
      if (!this.wifi_ssid.length)
        return this.$root.error_dialog('SSID not set')

      if (32 < this.wifi_ssid.length)
        return this.$root.error_dialog('SSID longer than 32 characters')

      if (this.wifi_pass.length && this.wifi_pass.length < 8)
        return this.$root.error_dialog(
          'WiFi password shorter than 8 characters')

      if (128 < this.wifi_pass.length)
        return this.$root.error_dialog(
          'WiFi password longer than 128 characters')

      let config = {
        mode:     this.wifi_mode,
        internal: this.wifi_internal,
        channel:  this.wifi_ch,
        ssid:     this.wifi_ssid,
        pass:     this.wifi_pass
      }

      return this.$api.put('wifi', config)
    },


    async set_country() {
      return this.$api.put('net/country', {code: this.wifi_country})
    },


    async get_wifi_password() {
      this.wifi_pass = ''

      let result = await this.$refs.wifiPassword.open()
      if (result == 'ok') return this.wifi_pass
    },


    async connect(net) {
      let result = await this.$refs.connect.open(net)

      if (result.action == 'connect') {
        let ap = result.ap
        let data = {}

        if (ap.connection) data.uuid = ap.connection
        else {
          data.ssid = ap.ssid

          if (ap.security) {
            data.password = await this.get_wifi_password()
            if (!data.password) return
          }
        }

        return this.$api.put('net/' + net.name + '/connect', data)
      }
    },


    hotspot(net) {this.$refs.hotspot.open(net)},


    async disconnect(net) {
      return this.$api.put('net/' + net.name + '/disconnect')
    }
  }
}
