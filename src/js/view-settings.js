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

        let authorized = this.$root.authorized
        let result = await this.$root.open_dialog({
          header: authorized ? 'Save settings?' : 'Login required',
          body:   authorized ?
            'Changes to the settings have not been saved. Would you like to ' +
            'save them now?' :
            'Changes to the settings have not been saved. Log in to save ' +
            'them, discard them, or stay on this page.',
          width:  '320px',
          buttons: authorized ? [
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
          ] : [
            {
              text:  'Cancel',
              title: 'Stay on the Settings page.'
            }, {
              text:  'Discard',
              title: 'Discard settings changes.'
            }, {
              text:  'Login',
              title: 'Log in and save settings.',
              class: 'button-success'
            }
          ]})

        if (result == 'cancel') return
        if (result == 'discard') {
          await this.discard()
          return location.hash = path.join(':')
        }

        if (result == 'save' || result == 'login') {
          if (await this.save()) location.hash = path.join(':')
        }
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
    async authorize() {
      if (this.$root.authorized) return true
      await this.$root.login()
      return this.$root.authorized
    },


    get_error_message(error, fallback) {
      let xhr = error && error.xhr

      if (xhr && xhr.response && xhr.response.message)
        return xhr.response.message

      if (xhr && xhr.responseText) {
        try {
          let response = JSON.parse(xhr.responseText)
          if (response && response.message) return response.message
        } catch (e) {}
      }

      if (xhr && xhr.statusText) return xhr.statusText
      return fallback
    },


    async save(retry = true) {
      if (!await this.authorize()) return false

      try {
        await this.$api.put('config/save', this.config, {error() {}})
        this.modified = false
        return true

      } catch (error) {
        let xhr = error && error.xhr

        if (xhr && xhr.status == 401) {
          this.$root.authorized = false

          if (retry) {
            let result = await this.$root.open_dialog({
              header: 'Login required',
              body: 'Your login session expired. Log in to save settings.',
              buttons: [
                {text: 'Cancel'},
                {text: 'Login', class: 'button-success'}
              ]
            })

            if (result == 'login' && await this.authorize())
              return this.save(false)
          }

          return false
        }

        await this.$root.error_dialog('Failed to save settings.\n' +
          this.get_error_message(error, 'Unable to save configuration.'))
        return false
      }
    },


    async discard() {
      await this.$root.update()
      this.modified = false
    }
  }
}
