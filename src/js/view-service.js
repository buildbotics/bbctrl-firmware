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

let serviceTemplate = require('../resources/service-template.json')

module.exports = {
  template: '#view-service-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      view: 'dashboard',
      serviceTemplate: serviceTemplate,
      serviceData: {
        power_hours: 0,
        spindle_hours: 0,
        motion_hours: 0,
        items: [],
        notes: []
      },
      editingItem: null,
      editingNote: null,
      currentItemId: null,
      modified: false,
      originalItem: null,
      originalNote: null,
      manualEntry: {
        hours: 0,
        date: '',
        notes: ''
      }
    }
  },


  computed: {
    powerHours() {
      return (this.state.service_power_hours || this.serviceData.power_hours || 0).toFixed(1)
    },
    
    spindleHours() {
      return (this.state.service_spindle_hours || this.serviceData.spindle_hours || 0).toFixed(1)
    },
    
    motionHours() {
      return (this.state.service_motion_hours || this.serviceData.motion_hours || 0).toFixed(1)
    },

    itemsSorted() {
      return [...this.serviceData.items].sort((a, b) => {
        // Due items first, then by hours remaining
        let aRemaining = a.next_due - this.getHoursForType(a.hour_type)
        let bRemaining = b.next_due - this.getHoursForType(b.hour_type)
        return aRemaining - bRemaining
      })
    },

    dueItems() {
      return this.itemsSorted.filter(item => this.isItemDue(item))
    },

    notesSorted() {
      return [...this.serviceData.notes].sort((a, b) => 
        new Date(b.created) - new Date(a.created)
      )
    },
    
    hourTypes() {
      return [
        {value: 'motion_hours', label: 'Motion Hours'},
        {value: 'spindle_hours', label: 'Spindle Hours'},
        {value: 'power_hours', label: 'Power-On Hours'}
      ]
    }
  },


  events: {
    route(path) {
      if (path[0] != 'service') return
      let view = path.length < 2 ? 'dashboard' : path[1]
      
      if (!['dashboard', 'items', 'notes'].includes(view)) {
        return this.$root.replace_route('service:dashboard')
      }
      
      this.view = view
    },
    
    
    'route-changing'(path, cancel) {
      // Don't block if staying on service pages
      if (path[0] == 'service') return
      
      // Check for unsaved editor changes
      if (this.modified || this.editingItem || this.editingNote) {
        cancel()
        this.checkUnsavedChanges(() => {
          location.hash = path.join(':')
        })
      }
    }
  },


  ready() {
    this.loadService()
    this.$root.parse_hash()
  },


  methods: {
    async loadService() {
      try {
        this.serviceData = await this.$api.get('service')
      } catch (error) {
        console.error('Failed to load service data:', error)
      }
    },
    
    
    // === DIRTY FORM TRACKING ===
    
    markModified() {
      this.modified = true
    },
    
    
    clearModified() {
      this.modified = false
      this.originalItem = null
      this.originalNote = null
    },
    
    
    async checkUnsavedChanges(callback) {
      let response = await this.$root.open_dialog({
        header: 'Unsaved Changes',
        body: 'You have unsaved changes. Would you like to save them first?',
        width: '320px',
        buttons: [
          {text: 'Cancel', title: 'Stay on this page'},
          {text: 'Discard', title: 'Discard changes'},
          {text: 'Save', title: 'Save changes', class: 'button-success'}
        ]
      })
      
      if (response == 'save') {
        await this.saveChanges()
        if (callback) callback()
      } else if (response == 'discard') {
        this.discardChanges()
        if (callback) callback()
      }
      // Cancel does nothing - stays on page
    },
    
    
    async saveChanges() {
      if (this.editingItem) await this.saveItem()
      if (this.editingNote) await this.saveNote()
      this.clearModified()
    },
    
    
    discardChanges() {
      this.editingItem = null
      this.editingNote = null
      this.clearModified()
    },
    
    
    getHoursForType(hourType) {
      let type = hourType || 'motion_hours'
      if (type === 'power_hours') return parseFloat(this.powerHours)
      if (type === 'spindle_hours') return parseFloat(this.spindleHours)
      return parseFloat(this.motionHours)
    },
    
    
    getHourTypeLabel(hourType) {
      let labels = {
        'power_hours': 'Power',
        'spindle_hours': 'Spindle',
        'motion_hours': 'Motion'
      }
      return labels[hourType] || 'Motion'
    },


    // === SERVICE ITEMS ===

    async addItem() {
      let result = await this.$root.open_dialog({
        header: 'Add Service Item',
        body: this.getItemDialogBody(),
        width: '400px',
        buttons: [
          {text: 'Cancel'},
          {text: 'Add', class: 'button-success', action: 'add'}
        ]
      })
      
      if (result == 'add') {
        await this.saveNewItem()
      }
    },
    
    
    getItemDialogBody() {
      this.editingItem = {
        label: '',
        interval: 100,
        color: '#e6e6e6',
        hour_type: 'motion_hours'
      }
      
      // Return simple form - complex dialogs will use the built-in dialog component
      return 'Use the Items page to configure service items with full options.'
    },


    async openItemEditor(item) {
      // Check for unsaved changes in note editor
      if (this.editingNote && this.modified) {
        let response = await this.$root.open_dialog({
          header: 'Unsaved Note',
          body: 'You have an unsaved note. Discard changes?',
          buttons: [
            {text: 'Cancel'},
            {text: 'Discard', action: 'discard'}
          ]
        })
        if (response != 'discard') return
        this.editingNote = null
      }
      
      this.editingItem = item ? {...item} : {
        label: '',
        interval: 100,
        color: '#e6e6e6',
        hour_type: 'motion_hours'
      }
      this.originalItem = item ? {...item} : null
      this.modified = false
    },
    
    
    async cancelItemEdit() {
      if (this.modified) {
        let response = await this.$root.open_dialog({
          header: 'Discard Changes?',
          body: 'You have unsaved changes. Are you sure you want to cancel?',
          buttons: [
            {text: 'Keep Editing'},
            {text: 'Discard', class: 'button-danger', action: 'discard'}
          ]
        })
        if (response != 'discard') return
      }
      this.editingItem = null
      this.clearModified()
    },


    async saveItem() {
      if (!this.editingItem) return
      
      try {
        if (this.editingItem.id) {
          await this.$api.put('service/items/' + this.editingItem.id, this.editingItem)
        } else {
          await this.$api.post('service/items', this.editingItem)
        }

        await this.loadService()
        this.editingItem = null
        this.clearModified()
      } catch (error) {
        this.$root.error_dialog('Failed to save service item: ' + error)
      }
    },
    
    
    async saveNewItem() {
      if (!this.editingItem) return
      
      try {
        await this.$api.post('service/items', this.editingItem)
        await this.loadService()
        this.editingItem = null
      } catch (error) {
        this.$root.error_dialog('Failed to save service item: ' + error)
      }
    },


    async deleteItem(item) {
      let result = await this.$root.open_dialog({
        header: 'Delete Service Item',
        body: 'Delete "' + item.label + '"? This cannot be undone.',
        buttons: [
          {text: 'Cancel'},
          {text: 'Delete', class: 'button-danger', action: 'delete'}
        ]
      })
      
      if (result != 'delete') return

      try {
        await this.$api.delete('service/items/' + item.id)
        await this.loadService()
      } catch (error) {
        this.$root.error_dialog('Failed to delete service item: ' + error)
      }
    },


    getItemHoursRemaining(item) {
      let currentHours = this.getHoursForType(item.hour_type)
      let remaining = item.next_due - currentHours
      if (remaining <= 0) return 'DUE NOW'
      return remaining.toFixed(1) + ' hrs'
    },


    isItemDue(item) {
      let currentHours = this.getHoursForType(item.hour_type)
      return currentHours >= item.next_due
    },


    async completeItem(item) {
      let result = await this.$root.open_dialog({
        header: 'Complete Service',
        body: 'Mark "' + item.label + '" as completed at ' + 
              this.getHoursForType(item.hour_type).toFixed(1) + ' hours?',
        buttons: [
          {text: 'Cancel'},
          {text: 'Complete', class: 'button-success', action: 'complete'}
        ]
      })
      
      if (result != 'complete') return

      try {
        await this.$api.put('service/complete/' + item.id, {notes: ''})
        await this.loadService()
        this.$root.success_dialog('Service completed! Next due at ' + 
          (this.getHoursForType(item.hour_type) + item.interval).toFixed(1) + ' hours.')
      } catch (error) {
        this.$root.error_dialog('Failed to complete service: ' + error)
      }
    },


    // === HISTORY ===

    getCurrentItem() {
      return this.serviceData.items.find(item => item.id === this.currentItemId)
    },


    getHistory() {
      let item = this.getCurrentItem()
      return item ? (item.history || []) : []
    },


    formatDate(dateStr) {
      if (!dateStr) return ''
      let date = new Date(dateStr)
      return date.toLocaleDateString() + ' ' + date.toLocaleTimeString()
    },


    // === NOTES ===

    async openNoteEditor(note) {
      // Check for unsaved changes in item editor
      if (this.editingItem && this.modified) {
        let response = await this.$root.open_dialog({
          header: 'Unsaved Item',
          body: 'You have an unsaved service item. Discard changes?',
          buttons: [
            {text: 'Cancel'},
            {text: 'Discard', action: 'discard'}
          ]
        })
        if (response != 'discard') return
        this.editingItem = null
      }
      
      this.editingNote = note ? {...note} : {
        title: '',
        content: ''
      }
      this.originalNote = note ? {...note} : null
      this.modified = false
    },
    
    
    async cancelNoteEdit() {
      if (this.modified) {
        let response = await this.$root.open_dialog({
          header: 'Discard Changes?',
          body: 'You have unsaved changes. Are you sure you want to cancel?',
          buttons: [
            {text: 'Keep Editing'},
            {text: 'Discard', class: 'button-danger', action: 'discard'}
          ]
        })
        if (response != 'discard') return
      }
      this.editingNote = null
      this.clearModified()
    },


    async saveNote() {
      if (!this.editingNote) return
      
      try {
        if (this.editingNote.id) {
          await this.$api.put('service/notes/' + this.editingNote.id, this.editingNote)
        } else {
          await this.$api.post('service/notes', this.editingNote)
        }

        await this.loadService()
        this.editingNote = null
        this.clearModified()
      } catch (error) {
        this.$root.error_dialog('Failed to save note: ' + error)
      }
    },


    async deleteNote(note) {
      let result = await this.$root.open_dialog({
        header: 'Delete Note',
        body: 'Delete "' + note.title + '"?',
        buttons: [
          {text: 'Cancel'},
          {text: 'Delete', class: 'button-danger', action: 'delete'}
        ]
      })
      
      if (result != 'delete') return

      try {
        await this.$api.delete('service/notes/' + note.id)
        await this.loadService()
      } catch (error) {
        this.$root.error_dialog('Failed to delete note: ' + error)
      }
    },


    // === EXPORT ===

    exportData() {
      window.open('/api/service/export', '_blank')
    }
  }
}
