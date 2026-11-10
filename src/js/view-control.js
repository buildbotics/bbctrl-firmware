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


let cookie = require('./cookie')
let util   = require('./util')


module.exports = {
  template: '#view-control-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      mdi: '',
      history: [],
      history_index: -1,
      jog_step: cookie.get_bool('jog-step'),
      jog_adjust: parseInt(cookie.get('jog-adjust', 2)),
      highlighted_line: 0,
      loading: false,
      toolpath: {},
      snaps: 'angled top bottom front back left right'.split(' ')
    }
  },


  components: {
    'axis-row':           require('./axis-row'),
    'axis-control':       require('./axis-control'),
    'path-viewer':        require('./path-viewer'),
    'viewer-help-dialog': require('./viewer-help-dialog')
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


    // 3D view options, owned by the path viewer
    show() {
      if (this.$refs.viewer == undefined) return {}
      return this.$refs.viewer.show
    },


    // Units display with G-code reference
    mach_units() {
      return this.state.imperial ? 'Imperial (G20)' : 'Metric (G21)'
    },


    distance_mode() {
      return this.state.incremental_distance_mode ?
        'Incremental (G91)' : 'Absolute (G90)'
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
