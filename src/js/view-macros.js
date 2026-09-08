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


let util = require('./util')


module.exports = {
  template: '#view-macros-template',
  props: ['config', 'state', 'template'],


  data() {
    return {
      view: 'macros',
      dragging: -1,
      draggingTab: -1,
      modified: false
    }
  },


  computed: {
    macros() {return this.config.macros || []},


    macro_tabs() {
      if (!this.config.macro_tabs || !this.config.macro_tabs.length)
        this.config.macro_tabs = [{id: 'default', name: 'Macros'}]
      return this.config.macro_tabs
    },


    visible_macro_count() {
      let count = 0
      for (let i = 0; i < this.macros.length; i++) {
        if (this.macros[i].visible !== false) count++
      }
      return count
    }
  },


  events: {
    route(path) {
      if (path[0] != 'macros') return
      let view = path.length < 2 ? 'macros' : path[1]

      if (['macros', 'tabs', 'safety'].indexOf(view) == -1)
        return this.$root.replace_route('macros')

      this.view = view
    },


    async 'route-changing'(path, cancel) {
      if (this.modified && path[0] != 'macros') {
        cancel()

        let authorized = this.$root.authorized
        let result = await this.$root.open_dialog({
          header: authorized ? 'Save changes?' : 'Login required',
          body: authorized ?
            'Changes to macros have not been saved. Would you like to save ' +
            'them now?' :
            'Changes to macros have not been saved. Log in to save them, ' +
            'discard them, or stay on this page.',
          width: '320px',
          buttons: authorized ? [
            {
              text: 'Cancel',
              title: 'Stay on the Macros page.'
            }, {
              text: 'Discard',
              title: 'Discard changes.'
            }, {
              text: 'Save',
              title: 'Save changes.',
              class: 'button-success'
            }
          ] : [
            {
              text: 'Cancel',
              title: 'Stay on the Macros page.'
            }, {
              text: 'Discard',
              title: 'Discard changes.'
            }, {
              text: 'Login',
              title: 'Log in and save changes.',
              class: 'button-success'
            }
          ]
        })

        if (result == 'cancel') return
        if (result == 'discard') {
          await this.discard()
          return location.hash = path.join(':')
        }

        if (result == 'save' || result == 'login') {
          if (await this.save()) location.hash = path.join(':')
        }
      }
    }
  },


  ready() {
    this.$root.parse_hash()
  },


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


    async add_tab() {
      if (!await this.authorize()) return
      let id = 'tab_' + Date.now()
      this.macro_tabs.push({id: id, name: 'New Tab'})
      this.change()
    },


    async remove_tab(index) {
      if (!await this.authorize()) return
      let tab = this.macro_tabs[index]
      if (!tab) return

      if (this.macro_tabs.length <= 1) {
        this.$root.error_dialog('Cannot remove the last tab. At least one tab ' +
          'is required.')
        return
      }

      let count = this.count_macros_in_tab(tab.id)
      let body = 'Delete tab "' + tab.name + '"?'
      if (count) body += '\n\n' + count + ' macro(s) will be moved to the first tab.'

      let result = await this.$root.open_dialog({
        header: 'Delete Tab',
        body: body,
        buttons: [
          {text: 'Cancel'},
          {text: 'Delete', class: 'button-danger', action: 'delete'}
        ]
      })

      if (result != 'delete') return

      let first = this.macro_tabs[0].id
      if (this.macro_tabs[0].id == tab.id && 1 < this.macro_tabs.length)
        first = this.macro_tabs[1].id

      for (let i = 0; i < this.macros.length; i++) {
        if (this.macros[i].tab == tab.id) this.macros[i].tab = first
      }

      this.macro_tabs.splice(index, 1)
      this.change()
    },


    count_macros_in_tab(tabId) {
      let count = 0
      let first = this.macro_tabs.length ? this.macro_tabs[0].id : 'default'

      for (let i = 0; i < this.macros.length; i++) {
        let tab = this.macros[i].tab || first
        if (tab == tabId) count++
      }

      return count
    },


    tab_mousedown(event) {this.tabTarget = event.target},


    tab_dragstart(event) {
      if (this.tabTarget && this.tabTarget.localName == 'input')
        event.preventDefault()
    },


    tab_drag(index, event) {
      this.draggingTab = index
      event.preventDefault()
    },


    tab_drop(index) {
      if (!this.$root.authorized) return
      if (index == this.draggingTab) return
      let item = this.macro_tabs[this.draggingTab]
      this.macro_tabs.splice(this.draggingTab, 1)
      this.macro_tabs.splice(index, 0, item)
      this.change()
    },


    async add() {
      if (!await this.authorize()) return
      let tab = this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
      this.macros.push({
        name: '',
        path: '',
        color: '#e6e6e6',
        tab: tab,
        visible: true,
        confirm: true
      })
      this.change()
    },


    is_visible(macro) {
      return macro && macro.visible !== false
    },


    async toggle_visibility(index) {
      if (!await this.authorize()) return
      let macro = this.macros[index]
      if (!macro) return
      macro.visible = !this.is_visible(macro)
      this.change()
    },


    requires_confirm(macro) {
      return macro && macro.confirm !== false
    },


    async toggle_confirm(index) {
      if (!await this.authorize()) return
      let macro = this.macros[index]
      if (!macro) return
      macro.confirm = !this.requires_confirm(macro)
      this.change()
    },


    get_macro_tab(macro) {
      if (macro.tab) return macro.tab
      return this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
    },


    async set_macro_tab(index, tabId) {
      if (!await this.authorize()) return
      let macro = this.macros[index]
      if (!macro) return
      macro.tab = tabId
      this.change()
    },


    mousedown(event) {this.target = event.target},


    dragstart(event) {
      if (this.target &&
          (this.target.localName == 'input' ||
           this.target.localName == 'select'))
        event.preventDefault()
    },


    drag(index, event) {
      this.dragging = index
      event.preventDefault()
    },


    drop(index) {
      if (!this.$root.authorized) return
      if (index == this.dragging) return
      let item = this.macros[this.dragging]
      this.macros.splice(this.dragging, 1)
      this.macros.splice(index, 0, item)
      this.change()
    },


    async remove(index) {
      if (!await this.authorize()) return
      let macro = this.macros[index]
      let name = macro.name || ('Macro ' + (index + 1))

      let result = await this.$root.open_dialog({
        header: 'Delete Macro',
        body: 'Delete "' + name + '"? This cannot be undone.',
        buttons: [
          {text: 'Cancel'},
          {text: 'Delete', class: 'button-danger', action: 'delete'}
        ]
      })

      if (result != 'delete') return

      this.macros.splice(index, 1)
      this.change()
    },


    async open(index) {
      if (!await this.authorize()) return
      let path = await this.$root.file_dialog()
      if (!path) return
      this.macros[index].path = util.display_path(path)
      this.change()
    },


    change() {
      if (!this.$root.authorized) return
      this.modified = true
      this.$dispatch('input-changed')
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
              body: 'Your login session expired. Log in to save macro ' +
                    'changes.',
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

        await this.$root.error_dialog('Failed to save macro changes.\n' +
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
