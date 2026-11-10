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


// Matches src/py/bbctrl/OffsetTable.py.  Values travel to and from the
// controller in millimeters and are shown in the current units.
const WORK_SYSTEMS = ['G54', 'G55', 'G56', 'G57', 'G58', 'G59']
const PROBE_TOOL   = 99


function range(first, last) {
  let list = []
  for (let i = first; i <= last; i++) list.push(i)
  return list
}


module.exports = {
  template: '#view-offsets-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      tab: 'work',
      systems: WORK_SYSTEMS,
      tools: range(1, PROBE_TOOL),
      probe_tool: PROBE_TOOL
    }
  },


  events: {
    route(path) {this.route(path)}
  },


  // The route event may come before this view exists
  ready() {this.route(location.hash.substr(1).split(':'))},


  computed: {
    metric() {return !this.state.imperial},
    units() {return this.metric ? 'mm' : 'in'},
    table() {return this.state.offset_table},


    // Offsets are stored by the planner, which must not be mid-program
    editable() {return this.state.cycle == 'idle'},


    // Switching coordinate systems runs G-code
    can_mdi() {return this.editable && this.state.xx == 'READY'},


    // Axes with an enabled motor, in XYZABC order, or XYZ if none is set up
    work_axes() {
      let enabled = {}
      for (let motor of this.config.motors)
        if (motor.enabled && motor.axis)
          enabled[motor.axis.toLowerCase()] = true

      let axes = 'xyzabc'.split('').filter(axis => enabled[axis])
      return axes.length ? axes : ['x', 'y', 'z']
    },


    active_system() {
      let cs = this.state.coord_system || 540
      return cs <= 590 ? 'G' + Math.floor(cs / 10) : 'G59.' + (cs - 590)
    },


    machine_z() {return this.state.zp},
    has_machine_z() {return typeof this.state.zp == 'number'}
  },


  methods: {
    route(path) {
      if (path[0] == 'offsets' && (path[1] == 'work' || path[1] == 'tool'))
        this.tab = path[1]
    },


    show_tab(tab) {this.tab = tab},


    format(mm) {
      if (typeof mm != 'number') return ''
      return this.metric ? mm.toFixed(3) : (mm / 25.4).toFixed(4)
    },


    // Text typed in the current units to millimeters, or undefined
    parse(text) {
      text = ('' + text).trim()
      if (!/^[-+]?(\d+\.?\d*|\.\d+)$/.test(text)) return
      let value = parseFloat(text)
      return this.metric ? value : value * 25.4
    },


    async edit(changes, input, old_mm) {
      try {
        await this.$api.put('offsets', changes)
      } catch (e) {
        input.value = this.format(old_mm) // The API reports the error
      }
    },


    // Restores the shown value and returns undefined if the text is invalid
    read_input(e, old_mm) {
      let mm = this.parse(e.target.value)
      if (typeof mm == 'undefined' || mm == old_mm) {
        e.target.value = this.format(old_mm)
        return
      }
      return mm
    },


    set_work(cs, axis, e) {
      let old_mm = this.table.work[cs][axis]
      let mm = this.read_input(e, old_mm)
      if (typeof mm == 'undefined') return

      let work = {}
      work[cs] = {}
      work[cs][axis] = mm
      this.edit({work}, e.target, old_mm)
    },


    set_tool(tool, e) {
      let old_mm = this.table.tools[tool].z
      let mm = this.read_input(e, old_mm)
      if (typeof mm == 'undefined') return

      let tools = {}
      tools[tool] = {z: mm}
      this.edit({tools}, e.target, old_mm)
    },


    set_reference(e) {
      let old_mm = this.table.tool_reference.z
      let mm = this.read_input(e, old_mm)
      if (typeof mm == 'undefined') return
      this.edit({tool_reference: {z: mm}}, e.target, old_mm)
    },


    reference_from_machine() {
      this.$api.put('offsets', {tool_reference: {z: this.machine_z}})
    },


    send(gcode) {this.$dispatch('send', gcode)},


    activate(cs) {this.send(cs)}
  }
}
