/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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
  template: '#chart-view-template',
  props: ['config', 'template', 'state'],


  data() {
    return {
      v: [],
      s: [],
    }
  },


  ready() {
    let options = {
      title: {display: true},
      legend: {display: false},
      animation: {duration: 100},

      scales: {
        xAxes: [{
          type: 'time',
          time: {
            unit: 'hour',
            displayFormats: {hour: 'MMM Do, H:00'}
          }
        }],

        yAxes: [{
          ticks: {
            beginAtZero: true,
            callback(value) {
              return value.toLocaleString()
            }
          }
        }]
      }
    }

    this.chart = new Chart(this.$els.chart, {
      type: 'line',
      data: {datasets: [
        {
          lineTension: 0,
          pointRadius: 0,
          data: this.v

        }, {
          backgroundColor: '#ff0000',
          lineTension: 0,
          pointRadius: 0,
          data: this.s
        }
      ]},
      options: options
    })


    setInterval(() => {this.add(this.state.v, this.state['0sl'])}, 125)
  },


  methods: {
    add(v, s) {
      let t = new Date()

      this.v.unshift({x: t, y: v})
      this.s.unshift({x: t, y: s})

      if (40 < this.v.length) this.v.length = 40
      if (40 < this.s.length) this.s.length = 40

      if (this.chart != undefined) this.chart.update()
    }
  }
}
