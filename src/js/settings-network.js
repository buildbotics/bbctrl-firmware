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
  template: '#settings-network-template',
  props: ['config', 'state'],


  components: {
    'wifi-connect': require('./wifi-connect'),
    'wifi-hotspot': require('./wifi-hotspot')
  },


  data() {
    return {
      redirectTimeout: 0,
      hostname:        '',
      wifi_pass:       '',
      wifi_region:     '',
      wifi_regions:    require('../resources/wifi-regions.json'),
    }
  },


  computed: {
    authorized() {return this.$root.authorized}
  },


  async ready() {
    this.hostname = await this.$api.get('hostname')
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
      this.$refs.hostnameSet.open()

      await this.$api.put('reboot')

      if (this.$root.is_local) return

      let hostname = this.hostname
      if (String(location.hostname).endsWith('.local'))
        hostname += '.local'
      this.$dispatch('hostname-changed', hostname)
      this.redirect(hostname)
    },


    async set_region() {
      return this.$api.put('wifi/region', {code: this.wifi_region})
    },


    async get_wifi_password() {
      this.wifi_pass = ''

      let result = await this.$refs.wifiPassword.open()
      if (result == 'ok') return this.wifi_pass
    },


    async connect(net) {
      let result = await this.$refs.connect.open(net)
      let password

      if (result.action == 'connect') {
        let data = {}

        if (result.ssid) {
          data.ssid = result.ssid
          password = await this.get_wifi_password()

        } else if (result.ap.connection) data.uuid = result.ap.connection
        else {
          data.ssid = result.ap.ssid

          if (result.ap.security) {
            password = await this.get_wifi_password()
            if (password == undefined) return
          }
        }

        if (password) data.password = password

        return this.$api.put('wifi/' + net.name + '/connect', data)
      }
    },


    hotspot(net) {this.$refs.hotspot.open(net)},


    async disconnect(net) {
      return this.$api.put('wifi/' + net.name + '/disconnect')
    }
  }
}
