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
  template: '#view-settings-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      index: -1,
      view: undefined,
      modified: false
    }
  },


  components: {
    'settings-general':   require('./settings-general'),
    'settings-motor':     require('./settings-motor'),
    'settings-tool':      require('./settings-tool'),
    'settings-io':        require('./settings-io'),
    'settings-macros':    require('./settings-macros'),
    'settings-network':   require('./settings-network'),
    'settings-admin':     require('./settings-admin')
  },


  events: {
    async 'route-changing'(path, cancel) {
      if (this.modified && path[0] != 'settings') {
        cancel()

        let result = await this.$root.open_dialog({
          header: 'Save settings?',
          body:   'Changes to the settings have not been saved.  ' +
                  'Would you like to save them now?',
          width:  '320px',
          buttons: [
            {
              text:  'Cancel',
              title: 'Stay on the Settings page.'
            }, {
              text:  'Discard',
              title: 'Discard settings changes.'
            }, {
              text:  'Save',
              title: 'Save settings.',
              class: 'button-success'
            }
          ]})

        if (result == 'cancel')  return
        if (result == 'save')    await this.save()
        if (result == 'discard') await this.discard()

        location.hash = path.join(':')
      }
    },


    route(path) {
      if (path[0] != 'settings') return
      let view = path.length < 2 ? '' : path[1]

      if (typeof this.$options.components['settings-' + view] == 'undefined')
        return this.$root.replace_route('settings:general')

      if (path.length == 3) this.index = path[2]
      this.view = view
    },


    'config-changed'() {
      this.modified = true
      return false
    },


    'input-changed'() {
      this.$dispatch('config-changed')
      return false
    }
  },


  ready() {this.$root.parse_hash()},


  methods: {
    async save() {
      await this.$api.put('config/save', this.config)
      this.modified = false
    },


    async discard() {
      await this.$root.update()
      this.modified = false
    }
  }
}
