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
  template: '#wifi-connect-template',
  name: 'wifi-connect',


  data() {
    return {
      net:  {},
      ssid: '',
    }
  },


  methods: {
    scan() {
      this.$api.put('wifi/' + this.net.name + '/scan')
      clearTimeout(this.timer)
      this.timer = setTimeout(() => this.scan(), 4000)
    },


    async open(net) {
      this.net  = net
      this.ssid = ''
      this.scan()
      return this.$refs.dialog.open()
    },


    closed() {clearTimeout(this.timer)},
    close(response) {this.$refs.dialog.close(response)},
    connect(e) {this.close({action: 'connect', ap: e})},


    async forget(e) {
      let path = 'wifi/' + this.net.name + '/forget'
      await this.$api.put(path, {uuid: e.connection})
      delete e.connection
    }
  }
}
