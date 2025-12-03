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
      this.editingItem = item ? {...item} : {
        label: '',
        interval: 100,
        color: '#e6e6e6',
        hour_type: 'motion_hours'
      }
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

    openNoteEditor(note) {
      this.editingNote = note ? {...note} : {
        title: '',
        content: ''
      }
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
