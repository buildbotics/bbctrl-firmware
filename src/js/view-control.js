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


let util   = require('./util')


module.exports = {
  template: '#view-control-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      mdi: '',
      history: [],
      history_index: -1,
      highlighted_line: 0,
      loading: false,
      toolpath: {},
      snaps: 'angled top bottom front back left right'.split(' '),
      viewer_tab: this.$root.webGLSupported ? '3d' : 'messages',
      unseen_level: '', // 'warning' or 'error' while Messages is hidden
      new_tool: ''
    }
  },


  components: {
    'axis-row':           require('./axis-row'),
    'jog-control':        require('./jog-control'),
    'spindle-control':    require('./spindle-control'),
    'path-viewer':        require('./path-viewer'),
    'viewer-help-dialog': require('./viewer-help-dialog')
  },


  watch: {
    'state.line'() {
      if (this.mach_state != 'HOMING') this.highlight_code()
    },


    'active.path'() {this.load()},


    viewer_tab(tab) {
      if (tab == 'messages') this.unseen_level = ''

      // The canvas has no size while hidden
      if (tab == '3d' && this.$refs.viewer)
        Vue.nextTick(this.$refs.viewer.update_view)
    }
  },


  computed: {
    active() {
      return this.$root.active_program || this.$root.selected_program
    },


    metric() {return !this.state.imperial},


    // The modal codes the planner reports as they change.  Feed and spindle
    // modes, canned cycle retract and cutter compensation are not reported,
    // so they are not shown.  Until a program or MDI sets them, these are the
    // planner's defaults.
    modal_codes() {
      let state = this.state
      let codes = []

      // 540 for G54 through 590 for G59, 591-593 for G59.1-G59.3, published
      // by patches/camotics/0001-publish-coord-system.patch
      let cs = state.coord_system || 540
      codes.push({code: 'G' + cs / 10, title: 'Work coordinate system'})

      if (state.imperial) codes.push({code: 'G20', title: 'Inches'})
      else codes.push({code: 'G21', title: 'Millimeters'})

      if (state.incremental_distance_mode)
        codes.push({code: 'G91', title: 'Incremental distance'})
      else codes.push({code: 'G90', title: 'Absolute distance'})

      // CAMotics numbers the planes 170 for G17, 181 for G18.1 and so on
      let plane = state.plane || 170
      let planes = {170: 'XY', 171: 'UV', 180: 'ZX', 181: 'WU', 190: 'YZ',
                    191: 'VW'}
      codes.push({code: 'G' + plane / 10, title: planes[plane] + ' plane'})

      if (state.mist) codes.push({code: 'M7', title: 'Mist coolant on'})
      if (state.flood) codes.push({code: 'M8', title: 'Flood coolant on'})
      if (!state.mist && !state.flood)
        codes.push({code: 'M9', title: 'Coolant off'})

      return codes
    },


    // Stream only while the Camera tab is visible.  Views are kept alive, so
    // also stop when another page is shown or the camera modal has the feed.
    camera_live() {
      return this.viewer_tab == 'camera' &&
        this.$root.currentView == 'control' && !this.$root.cameraOpen
    },


    // 3D view options, owned by the path viewer
    show() {
      if (this.$refs.viewer == undefined) return {}
      return this.$refs.viewer.show
    },


    mach_state() {
      let cycle = this.state.cycle
      let state = this.state.xx

      if (typeof cycle != 'undefined' && state != 'ESTOPPED' &&
          (cycle == 'jogging' || cycle == 'homing'))
        return cycle.toUpperCase()
      return state || ''
    },


    eta() {
      if (!this.remaining || this.remaining <= 0) return null
      let eta = new Date(Date.now() + this.remaining * 1000)
      return eta.toLocaleTimeString([], {hour: '2-digit', minute: '2-digit'})
    },


    pause_reason() {return this.state.pr},


    is_running() {
      return this.mach_state == 'RUNNING' || this.mach_state == 'HOMING'
    },


    is_stopping() {return this.mach_state  == 'STOPPING'},
    is_holding()  {return this.mach_state  == 'HOLDING'},
    is_ready()    {return this.mach_state  == 'READY'},
    is_idle()     {return this.state.cycle == 'idle'},


    // A program or MDI command is underway, so the run button stops it
    program_active() {
      return this.is_running || this.is_holding || this.is_stopping
    },


    m01_break() {return !!this.state.optional_pause},


    // Reset clears an E-stop, or stops and rewinds an active program
    can_reset() {return this.mach_state == 'ESTOPPED' || this.program_active},


    is_paused() {
      return this.is_holding &&
        (this.pause_reason == 'User pause' ||
         this.pause_reason == 'Program pause')
    },


    can_mdi() {return this.is_idle || this.state.cycle == 'mdi'},


    // Blank or anything but a whole number is refused rather than guessed at
    new_tool_valid() {return /^\s*\d+\s*$/.test(this.new_tool)},


    // The reasons may not have arrived yet.  An undefined message throws in
    // the template, and in Vue 1 that stops every later update to the page.
    message() {
      if (this.mach_state == 'ESTOPPED') return this.state.er || ''
      if (this.mach_state == 'HOLDING') return this.state.pr || ''
      if (this.state.messages.length)
        return this.state.messages.slice(-1)[0].text
      return ''
    },


    // Colour of the state overlay
    state_class() {
      switch (this.mach_state) {
      case 'READY':    return 'ready'
      case 'RUNNING':
      case 'JOGGING':
      case 'HOMING':   return 'moving'
      case 'STOPPING':
      case 'HOLDING':  return 'holding'
      case 'ESTOPPED': return 'estopped'
      default:         return 'unknown'
      }
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


    simulating() {
      return 0 < this.active.progress && this.active.progress < 1
    },


    progress() {
      if (this.simulating) return this.active.progress

      if (!this.toolpath.time || this.is_ready) return 0
      let p = this.plan_time / this.toolpath.time
      return p < 1 ? p : 1
    }
  },


  events: {
    // Flag warnings and errors that arrive while the Messages tab is hidden.
    // Errors also bring the tab to the front, unless a program is running
    // and the 3D view is being watched; the error dialog still pops up.
    // Return true so the console below also gets the message.
    log(msg) {
      let level = msg.level || 'info'
      let error = level == 'error' || level == 'critical'

      if (this.viewer_tab == 'messages') return true

      if (error && !this.program_active) this.viewer_tab = 'messages'
      else if (error) this.unseen_level = 'error'
      else if (level == 'warning' && this.unseen_level != 'error')
        this.unseen_level = 'warning'

      return true
    },


    jog(axis, power) {
      let data = {ts: new Date().getTime()}
      data[axis] = power
      this.$api.put('jog', data)
    },


    step(axis, value) {
      this.send('M70\nG91\nG0' + axis + value + '\nM72')
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


  attached() {
    if (this.editor) this.editor.refresh()

    // The canvas has no size while the page is hidden
    if (this.$refs.viewer) Vue.nextTick(this.$refs.viewer.update_view)
  },


  methods: {
    // From axis-vars
    get_bounds() {return this.toolpath.bounds},


    goto(hash) {window.location.hash = hash},
    send(msg) {this.$dispatch('send', msg)},


    // Runs the same M6 a program would: the planner replaces it with the
    // tool-change GCode from Settings, so a hand change, tool setter probe
    // or ATC behaves the same from here as mid-program.
    async change_tool() {
      if (!this.is_idle) return
      this.new_tool = ''

      let response = await this.$refs.toolChange.open()
      if (response != 'change' || !this.new_tool_valid || !this.is_idle) return

      this.send('T' + parseInt(this.new_tool) + ' M6')
    },


    confirm_tool_change() {
      if (this.new_tool_valid) this.$refs.toolChange.close('change')
    },
    on_scroll(cm, e) {e.preventDefault()},


    async run_macro(macro) {
      try {
        return this.$api.put('macro/' + macro)
      } catch (e) {
        this.$root.error_dialog('Failed to run macro "' + macro + '":\n' + e)
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


    clear_display() {
      if (this.editor != undefined) this.editor.setValue('')
      if (this.$refs.viewer) this.$refs.viewer.clear()
      this.toolpath         = {}
      this.highlighted_line = 0
    },


    async load() {
      let path = this.active.path
      if (!path) return this.clear_display()

      let data = await this.active.load()
      if (this.active.path != path) return

      this.editor.setOption('mode', util.get_highlight_mode(path))
      this.editor.setValue(data)
      this.highlight_code()

      await this.load_toolpath(path)
    },


    // The 3D view also needs the positions and speeds, so only download
    // them when there is a viewer to show them
    async load_toolpath(path) {
      let program = this.active
      let viewer  = this.$refs.viewer
      if (viewer) viewer.clear()
      this.loading = true

      try {
        let toolpath = await (viewer ? program.view() : program.toolpath())
        if (this.active.path != path) return
        this.toolpath = toolpath
        if (viewer) Vue.nextTick(viewer.update)

      } finally {
        if (this.active.path == path) this.loading = false
      }
    },


    submit_mdi() {
      this.send(this.mdi)
      if (!this.history.length || this.history[0] != this.mdi)
        this.history.unshift(this.mdi)
      this.mdi = ''
      this.history_index = -1
    },


    // Up arrow steps back through earlier commands, down arrow forward
    history_back(e) {
      e.preventDefault()
      if (this.history_index + 1 < this.history.length)
        this.mdi = this.history[++this.history_index]
    },


    history_forward(e) {
      e.preventDefault()
      if (this.history_index < 0) return
      this.history_index--
      this.mdi = this.history_index < 0 ? '' : this.history[this.history_index]
    },


    mdi_start_pause() {
      if (this.state.xx == 'RUNNING') this.pause()

      else if (this.state.xx == 'STOPPING' || this.state.xx == 'HOLDING')
        this.unpause()

      else this.submit_mdi()
    },


    async home_all() {return this.$api.put('home')},


    run() {
      this.start().catch(() => {}) // Error already shown by API handler
    },


    async start()          {
      return this.$api.put('start/' + this.$root.selected_program.path)
    },


    async pause()          {return this.$api.put('pause')},
    async unpause()        {return this.$api.put('unpause')},
    async stop()           {return this.$api.put('stop')},


    async toggle_m01_break() {
      return this.$api.put('pause/optional', {enable: !this.m01_break})
    },


    // A stop already rewinds: the planner resets and the line returns to 0
    async reset() {
      if (this.mach_state == 'ESTOPPED') return this.$api.put('clear')
      if (this.program_active) return this.stop()
    },


    toggle(name) {this.$refs.viewer.toggle(name)},
    snap(view) {this.$refs.viewer.snap(view)},


    current(axis, value) {
      let x = value / 32.0
      if (this.state[axis + 'pl'] == x) return

      let data = {}
      data[axis + 'pl'] = x
      this.send(JSON.stringify(data))
    }
  },


  mixins: [require('./axis-vars')]
}
