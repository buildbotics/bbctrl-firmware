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
      // Ensure we always have at least a default tab
      if (!this.config.macro_tabs || !this.config.macro_tabs.length) {
        this.config.macro_tabs = [{id: 'default', name: 'Macros'}]
      }
      return this.config.macro_tabs
    },
    
    // Get count of visible macros
    visible_macro_count() {
      let count = 0
      for (let i = 0; i < this.macros.length; i++) {
        if (this.macros[i].visible !== false) count++
      }
      return count
    },
    
    // Get count of macros requiring confirmation
    confirm_macro_count() {
      let count = 0
      for (let i = 0; i < this.macros.length; i++) {
        if (this.macros[i].confirm !== false) count++
      }
      return count
    }
  },


  events: {
    route(path) {
      if (path[0] != 'macros') return
      let view = path.length < 2 ? 'macros' : path[1]
      
      if (!['macros', 'tabs'].includes(view)) {
        return this.$root.replace_route('macros')
      }
      
      this.view = view
    },
    
    
    async 'route-changing'(path, cancel) {
      if (this.modified && path[0] != 'macros') {
        cancel()

        let result = await this.$root.open_dialog({
          header: 'Save changes?',
          body: 'Changes to macros have not been saved. Would you like to save them now?',
          width: '320px',
          buttons: [
            {text: 'Cancel', title: 'Stay on the Macros page.'},
            {text: 'Discard', title: 'Discard changes.'},
            {text: 'Save', title: 'Save changes.', class: 'button-success'}
          ]
        })

        if (result == 'cancel') return
        if (result == 'save') await this.save()
        if (result == 'discard') await this.discard()

        location.hash = path.join(':')
      }
    }
  },


  ready() {
    this.$root.parse_hash()
  },


  methods: {
    // ===== TAB MANAGEMENT =====
    
    add_tab() {
      // Generate unique ID
      let id = 'tab_' + Date.now()
      this.macro_tabs.push({id: id, name: 'New Tab'})
      this.change()
    },
    
    
    async remove_tab(index) {
      let tab = this.macro_tabs[index]
      if (!tab) return
      
      // Don't allow removing the last tab
      if (this.macro_tabs.length <= 1) {
        this.$root.error_dialog('Cannot remove the last tab. At least one tab is required.')
        return
      }
      
      let macroCount = this.count_macros_in_tab(tab.id)
      let message = 'Delete tab "' + tab.name + '"?'
      if (macroCount > 0) {
        message += '\n\n' + macroCount + ' macro(s) will be moved to the first tab.'
      }
      
      let result = await this.$root.open_dialog({
        header: 'Delete Tab',
        body: message,
        buttons: [
          {text: 'Cancel'},
          {text: 'Delete', class: 'button-danger', action: 'delete'}
        ]
      })
      
      if (result != 'delete') return
      
      // Move macros from this tab to the first tab
      let firstTabId = this.macro_tabs[0].id
      if (this.macro_tabs[0].id === tab.id && this.macro_tabs.length > 1) {
        firstTabId = this.macro_tabs[1].id
      }
      
      for (let i = 0; i < this.macros.length; i++) {
        if (this.macros[i].tab === tab.id) {
          this.macros[i].tab = firstTabId
        }
      }
      
      this.macro_tabs.splice(index, 1)
      this.change()
    },
    
    
    // Count macros in a given tab
    count_macros_in_tab(tabId) {
      let count = 0
      let defaultTabId = this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
      
      for (let i = 0; i < this.macros.length; i++) {
        let macroTab = this.macros[i].tab || defaultTabId
        if (macroTab === tabId) count++
      }
      
      return count
    },
    
    
    tab_mousedown(event) {this.tabTarget = event.target},
    
    
    tab_dragstart(event) {
      if (this.tabTarget.localName == 'input') event.preventDefault()
    },
    
    
    tab_drag(index) {
      this.draggingTab = index
      event.preventDefault()
    },
    
    
    tab_drop(index) {
      if (index == this.draggingTab) return
      let item = this.macro_tabs[this.draggingTab]
      this.macro_tabs.splice(this.draggingTab, 1)
      this.macro_tabs.splice(index, 0, item)
      this.change()
    },
    
    
    // ===== MACRO MANAGEMENT =====
    
    add() {
      // Add macro to first tab by default
      let defaultTab = this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
      this.macros.push({
        name: '', 
        path: '', 
        color: '#e6e6e6',
        tab: defaultTab,
        visible: true,
        confirm: true
      })
      this.change()
    },


    is_visible(macro) {
      return macro && (macro.visible !== false)
    },


    toggle_visibility(index) {
      let macro = this.macros[index]
      if (macro) {
        macro.visible = !this.is_visible(macro)
        this.change()
      }
    },
    
    
    requires_confirm(macro) {
      return macro && (macro.confirm !== false)
    },
    
    
    toggle_confirm(index) {
      let macro = this.macros[index]
      if (macro) {
        macro.confirm = !this.requires_confirm(macro)
        this.change()
      }
    },
    
    
    get_macro_tab(macro) {
      if (macro.tab) return macro.tab
      return this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
    },
    
    
    set_macro_tab(index, tabId) {
      let macro = this.macros[index]
      if (macro) {
        macro.tab = tabId
        this.change()
      }
    },


    mousedown(event) {this.target = event.target},


    dragstart(event) {
      if (this.target.localName == 'input' || this.target.localName == 'select') {
        event.preventDefault()
      }
    },


    drag(index) {
      this.dragging = index
      event.preventDefault()
    },


    drop(index) {
      if (index == this.dragging) return
      let item = this.macros[this.dragging]
      this.macros.splice(this.dragging, 1)
      this.macros.splice(index, 0, item)
      this.change()
    },


    async remove(index) {
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
      let path = await this.$root.file_dialog()
      if (path) {
        this.macros[index].path = util.display_path(path)
        this.change()
      }
    },


    change() {
      this.modified = true
      this.$dispatch('input-changed')
    },
    
    
    async save() {
      try {
        await this.$api.put('config/save', this.config)
        this.modified = false
      } catch (error) {
        this.$root.error_dialog('Failed to save: ' + error)
      }
    },
    
    
    async discard() {
      await this.$root.update()
      this.modified = false
    }
  }
}
