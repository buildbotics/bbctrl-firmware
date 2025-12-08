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


let cookie = require('./cookie')
let util   = require('./util')


function _is_array(x) {
  return Object.prototype.toString.call(x) === '[object Array]'
}


function escapeHTML(s) {
  let entityMap = {'&': '&amp;', '<': '&lt;', '>': '&gt;'}
  return String(s).replace(/[&<>]/g, s => entityMap[s])
}


module.exports = {
  template: '#view-control-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      mdi: '',
      history: [],
      jog_step: cookie.get_bool('jog-step'),
      jog_adjust: parseInt(cookie.get('jog-adjust', 2)),
      tab: 'auto',
      highlighted_line: 0,
      toolpath: {},
      macro_tab: null  // Currently selected macro tab
    }
  },


  components: {
    'axis-row':     require('./axis-row'),
    'axis-control': require('./axis-control')
  },


  watch: {
    'state.line'() {
      if (this.mach_state != 'HOMING') this.highlight_code()
    },

    'active.path'() {this.load()},
    jog_step() {cookie.set_bool('jog-step', this.jog_step)},
    jog_adjust() {cookie.set('jog-adjust', this.jog_adjust)}
  },


  computed: {
    active() {
      return this.$root.active_program || this.$root.selected_program
    },


    metric() {return !this.state.imperial},


    // Units display with G-code reference
    mach_units() {
      return this.state.imperial ? 'Imperial (G20)' : 'Metric (G21)'
    },


    // Distance mode display with G-code reference
    // Backend broadcasts state.distance_mode: 90 = G90, 91 = G91
    distance_mode() {
      let mode = this.state.distance_mode
      return (mode == 91) ? 'Incremental (G91)' : 'Absolute (G90)'
    },


    mach_state() {
      let cycle = this.state.cycle
      let state = this.state.xx

      if (typeof cycle != 'undefined' && state != 'ESTOPPED' &&
          (cycle == 'jogging' || cycle == 'homing'))
        return cycle.toUpperCase()
      return state || ''
    },


    pause_reason() {return this.state.pr},


    is_running() {
      return this.mach_state == 'RUNNING' || this.mach_state == 'HOMING'
    },


    is_stopping() {return this.mach_state  == 'STOPPING'},
    is_holding()  {return this.mach_state  == 'HOLDING'},
    is_ready()    {return this.mach_state  == 'READY'},
    is_idle()     {return this.state.cycle == 'idle'},


    is_paused() {
      return this.is_holding &&
        (this.pause_reason == 'User pause' ||
         this.pause_reason == 'Program pause')
    },


    can_mdi() {return this.is_idle || this.state.cycle == 'mdi'},


    message() {
      if (this.mach_state == 'ESTOPPED') return this.state.er
      if (this.mach_state == 'HOLDING') return this.state.pr
      if (this.state.messages.length)
        return this.state.messages.slice(-1)[0].text
      return ''
    },


    highlight_state() {
      return this.mach_state == 'ESTOPPED' || this.mach_state == 'HOLDING'
    },


    plan_time() {return this.state.plan_time},


    remaining() {
      if (!(this.is_stopping || this.is_running || this.is_holding)) return 0
      if (this.toolpath.time < this.plan_time) return 0
      return this.toolpath.time - this.plan_time
    },


    // ETA: Estimated completion time based on remaining seconds
    // Uses browser locale for time format (12h vs 24h)
    eta() {
      if (!this.remaining || this.remaining <= 0) return null
      let completionTime = new Date(Date.now() + this.remaining * 1000)
      // Use short format: "2:45 PM" or "14:45" based on browser locale
      return completionTime.toLocaleTimeString([], {
        hour: '2-digit',
        minute: '2-digit'
      })
    },


    simulating() {
      return 0 < this.active.progress && this.active.progress < 1
    },


    progress() {
      if (this.simulating) return this.active.progress

      if (!this.toolpath.time || this.is_ready) return 0
      let p = this.plan_time / this.toolpath.time
      return p < 1 ? p : 1
    },
    
    
    // Get macro tabs from config, with default fallback
    macro_tabs() {
      let tabs = this.config.macro_tabs
      if (!tabs || !tabs.length) {
        return [{id: 'default', name: 'Macros'}]
      }
      return tabs
    },
    
    
    // Get the currently selected macro tab ID
    current_macro_tab() {
      if (this.macro_tab) return this.macro_tab
      return this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
    },
    
    
    // Get macros filtered by current tab and visibility
    visible_macros() {
      let macros = this.config.macros || []
      let currentTab = this.current_macro_tab
      let defaultTab = this.macro_tabs.length ? this.macro_tabs[0].id : 'default'
      
      let result = []
      for (let i = 0; i < macros.length; i++) {
        let macro = macros[i]
        // Check visibility
        if (macro.visible === false) continue
        // Check tab assignment
        let macroTab = macro.tab || defaultTab
        if (macroTab === currentTab) {
          result.push({
            name: macro.name,
            path: macro.path,
            color: macro.color,
            confirm: macro.confirm,
            originalIndex: i
          })
        }
      }
      return result
    },
    
    
    // Check if there are multiple tabs to show
    has_multiple_tabs() {
      return this.macro_tabs.length > 1
    },
    
  },


  events: {
    jog(axis, power) {
      let data = {ts: new Date().getTime()}
      data[axis] = power
      this.$api.put('jog', data)
    },


    step(axis, value) {
      this.send('M70\nG91\nG0' + axis + value + '\nM72')
    },
    
    
    // FIX #1: Handle program-cleared event to clear textarea
    'program-cleared'() {
      this.clear_display()
    },
    
    
    // FIX #5: Handle program-reloaded event to reload file content
    'program-reloaded'() {
      this.load()
    }
  },


  ready() {
    this.editor = CodeMirror.fromTextArea(this.$els.gcodeView, {
      readOnly: true,
      lineNumbers: true,
      mode: 'gcode'
    })

    this.editor.on('scrollCursorIntoView', this.on_scroll)
    this.load()
  },


  attached() {if (this.editor) this.editor.refresh()},


  methods: {
    // From axis-vars
    get_bounds() {return this.toolpath.bounds},


    goto(hash) {window.location.hash = hash},

    // Send G-code to backend
    // Distance mode (G90/G91) is tracked by CAMotics planner and broadcast via state
    send(msg) {this.$dispatch('send', msg)},

    on_scroll(cm, e) {e.preventDefault()},
    
    
    // Select a macro tab
    select_macro_tab(tabId) {
      this.macro_tab = tabId
    },


    async run_macro(macro) {
      try {
        // macro is the 1-based index from the button
        // originalIndex is the 0-based index in config.macros
        let originalIndex = macro
        if (typeof macro === 'object' && macro.originalIndex !== undefined) {
          originalIndex = macro.originalIndex
        }
        
        let macros = this.config.macros || []
        if (originalIndex < 0 || originalIndex >= macros.length) {
          throw new Error('Invalid macro index: ' + originalIndex)
        }
        
        let macroConfig = macros[originalIndex]
        if (!macroConfig || !macroConfig.path) {
          throw new Error('Macro has no file configured')
        }
        
        // Build full path
        let path = macroConfig.path
        if (!path.startsWith('Home/')) {
          path = 'Home/' + path
        }
        
        // SAFETY: Check position reference BEFORE showing confirmation
        // Don't ask user to confirm something they can't do anyway
        let unreferenced = this._get_unreferenced_axes()
        if (unreferenced.length) {
          this.$root.error_dialog(this._format_reference_error(unreferenced))
          return
        }
        
        // Check if confirmation is required (default: true for safety)
        let requiresConfirm = macroConfig.confirm !== false
        
        if (requiresConfirm) {
          // Show confirmation dialog before running macro
          let macroName = macroConfig.name || ('Macro ' + (originalIndex + 1))
          let confirmed = await this.$root.open_dialog({
            header: 'Confirm Macro',
            icon: 'question',
            body: 'Run macro "' + macroName + '"?\n\nFile: ' + path,
            buttons: [
              {text: 'Cancel', class: 'button-default'},
              {text: 'Run', class: 'button-success', action: 'run'}
            ]
          })
          
          // User cancelled - don't run macro
          if (confirmed != 'run') return
        }
        
        // Call the macro API endpoint (uses 1-based index)
        return this.$api.put('macro/' + (originalIndex + 1)).catch(() => {})
        
      } catch (e) {
        this.$root.error_dialog('Failed to run macro:\n' + e)
      }
    },


    highlight_code() {
      if (typeof this.editor == 'undefined') return
      let line = this.state.line - 1
      let doc = this.editor.getDoc()

      doc.removeLineClass(this.highlighted_line, 'wrap', 'highlight')

      if (0 <= line) {
        doc.addLineClass(line, 'wrap', 'highlight')
        this.highlighted_line = line
        this.editor.scrollIntoView({line: line, ch: 0}, 200)
      }
    },


    // FIX #1: Clear the display when no program is loaded
    clear_display() {
      if (typeof this.editor != 'undefined') {
        this.editor.setValue('')
      }
      this.toolpath = {}
      this.highlighted_line = 0
    },


    async load() {
      let path = this.active.path
      
      // FIX #1: If no path, clear the display
      if (!path) {
        this.clear_display()
        return
      }

      let data = await this.active.load()
      if (this.active.path != path) return

      this.editor.setOption('mode', util.get_highlight_mode(path))
      this.editor.setValue(data)
      this.highlight_code()

      let toolpath = await this.active.toolpath()
      if (this.active.path != path) return
      this.toolpath = toolpath
    },


    submit_mdi() {
      this.send(this.mdi)
      if (!this.history.length || this.history[0] != this.mdi)
        this.history.unshift(this.mdi)
      this.mdi = ''
    },


    mdi_start_pause() {
      if (this.state.xx == 'RUNNING') this.pause()

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause()

      else this.submit_mdi()
    },


    load_history(index) {this.mdi = this.history[index]},


    async home_all() {return this.$api.put('home')},


    zero_all() {
      for (let axis of 'xyzabc')
        if (this[axis].enabled)
          this.$api.put('position/' + axis, {position: 0})
    },


    start_pause() {
      if (this.state.xx == 'RUNNING') this.pause()

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause()

      else this.start().catch(() => {}) // Error already shown by API handler
    },


    async start()          {
      return this.$api.put('start/' + this.$root.selected_program.path)
    },


    async pause()          {return this.$api.put('pause')},
    async unpause()        {return this.$api.put('unpause')},
    async optional_pause() {return this.$api.put('pause/optional')},
    async stop()           {return this.$api.put('stop')},
    async step()           {return this.$api.put('step')},


    async open() {
      let aPath = this.active.path
      let dir   = aPath ? util.dirname(aPath) : '/'

      let path = await this.$root.file_dialog({dir})
      if (path) this.$root.select_path(path)
    },


    edit() {this.$root.edit(this.active.path)},
    view() {this.$root.view(this.active.path)},


    current(axis, value) {
      let x = value / 32.0
      if (this.state[axis + 'pl'] == x) return

      let data = {}
      data[axis + 'pl'] = x
      this.send(JSON.stringify(data))
    },


    // SAFETY: Check which axes lack position reference
    _get_unreferenced_axes() {
      let unreferenced = []
      let motors = this.config.motors || []
      
      for (let axis of 'xyzabc') {
        // Find motor for this axis
        let motor = -1
        for (let i = 0; i < motors.length; i++) {
          if (motors[i].axis && motors[i].axis.toLowerCase() == axis) {
            motor = i
            break
          }
        }
        
        if (motor == -1) continue
        if (!motors[motor].enabled) continue
        
        // Check if reference is required
        let refSetting = motors[motor]['reference-required'] || 'Auto'
        let required = (refSetting == 'Auto') ? 'xyz'.includes(axis) : (refSetting == 'Yes')
        
        if (required && !this.state[motor + 'referenced']) {
          unreferenced.push(axis.toUpperCase())
        }
      }
      
      return unreferenced
    },


    // Format user-friendly error message for unreferenced axes
    _format_reference_error(axes) {
      if (axes.length == 1) {
        return 'Cannot start: ' + axes[0] + ' axis position is unknown. ' +
               'Please home or zero the ' + axes[0] + ' axis before running.'
      } else if (axes.length == 2) {
        return 'Cannot start: ' + axes.join(' and ') + ' axis positions are unknown. ' +
               'Please home or zero these axes before running.'
      } else {
        return 'Cannot start: ' + axes.join(', ') + ' axis positions are unknown. ' +
               'Please home or zero all axes before running.'
      }
    }
  },


  mixins: [require('./axis-vars')]
}
