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
  template: '#axis-row-template',
  replace: true,
  props: ['axis', 'showOffset', 'showAbsolute'],


  data() {
    return {
      position: 0,
    }
  },


  computed: {
    name() {return this.axis.name.toUpperCase()},
    is_idle() {return this.$parent.is_idle}
  },


  methods: {
    async zero() {
      return this.$api.put('position/' + this.axis.name, {position: 0})
    },


    async set_position() {
      this.position = 0

      let response = await this.$refs.setPosition.open()

      if (response == 'set')
        return this.$api.put('position/' + this.axis.name,
                             {position: parseFloat(this.position)})

      if (response == 'unhome')
        return this.$api.put('home/' + this.axis.name + '/clear')
    },


    async home() {
      if (this.axis.homingMode != 'manual')
        return this.$api.put('home/' + this.axis.name)

      this.position = 0

      let response = await this.$refs.setHome.open()

      if (response == 'set')
        return this.$api.put('home/' + this.axis.name + '/set',
                             {position: parseFloat(this.position)})
    },
  }
}
