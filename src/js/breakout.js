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

'use strict'


function make_tmpl(name, data) {
  return {
    props: ['state', 'template'],
    template: '#breakout-' + name + '-template',
    data() {return Object.create(data || {})}
  }
}


module.exports = {
  template: '#breakout-template',
  props: ['name', 'state', 'template'],

  data() {return {active_pin: undefined}},


  components: {
    'db15-breakout': {'template': '#db15-breakout-template'},
    'db25-breakout': {'template': '#db25-breakout-template'},
    'db15-pin-1':    make_tmpl('nc'),
    'db15-pin-2':    make_tmpl('step', {ch: 0}),
    'db15-pin-3':    make_tmpl('step', {ch: 1}),
    'db15-pin-4':    make_tmpl('step', {ch: 2}),
    'db15-pin-5':    make_tmpl('step', {ch: 3}),
    'db15-pin-6':    make_tmpl('5v'),
    'db15-pin-7':    make_tmpl('5v'),
    'db15-pin-8':    make_tmpl('0to10'),
    'db15-pin-9':    make_tmpl('enable'),
    'db15-pin-10':   make_tmpl('dir', {ch: 0}),
    'db15-pin-11':   make_tmpl('dir', {ch: 1}),
    'db15-pin-12':   make_tmpl('dir', {ch: 2}),
    'db15-pin-13':   make_tmpl('dir', {ch: 3}),
    'db15-pin-14':   make_tmpl('ground'),
    'db15-pin-15':   make_tmpl('ground'),
    'db15-pin-gnd':  make_tmpl('ungrounded'),
    'db25-pin-1':    make_tmpl('io', {pin: 1}),
    'db25-pin-2':    make_tmpl('io', {pin: 2}),
    'db25-pin-3':    make_tmpl('io', {pin: 3}),
    'db25-pin-4':    make_tmpl('io', {pin: 4}),
    'db25-pin-5':    make_tmpl('io', {pin: 5}),
    'db25-pin-6':    make_tmpl('0to10'),
    'db25-pin-7':    make_tmpl('ground'),
    'db25-pin-8':    make_tmpl('io', {pin: 8}),
    'db25-pin-9':    make_tmpl('io', {pin: 9}),
    'db25-pin-10':   make_tmpl('io', {pin: 10}),
    'db25-pin-11':   make_tmpl('io', {pin: 11}),
    'db25-pin-12':   make_tmpl('io', {pin: 12}),
    'db25-pin-13':   make_tmpl('rs485', {ch: 'A'}),
    'db25-pin-14':   make_tmpl('rs485', {ch: 'B'}),
    'db25-pin-15':   make_tmpl('io', {pin: 15}),
    'db25-pin-16':   make_tmpl('io', {pin: 16}),
    'db25-pin-17':   make_tmpl('pwm'),
    'db25-pin-18':   make_tmpl('io', {pin: 18}),
    'db25-pin-19':   make_tmpl('ground'),
    'db25-pin-20':   make_tmpl('5v'),
    'db25-pin-21':   make_tmpl('io', {pin: 21}),
    'db25-pin-22':   make_tmpl('io', {pin: 22}),
    'db25-pin-23':   make_tmpl('io', {pin: 23}),
    'db25-pin-24':   make_tmpl('io', {pin: 24}),
    'db25-pin-25':   make_tmpl('ground'),
    'db25-pin-gnd':  make_tmpl('ground')
  },


  ready() {
    this.active_pin = '1'

    $(this.$refs.breakout.$el).find('.pin').mouseover((e) => {
      let classes = e.currentTarget.classList

      for (let i = 0; i < classes.length; i++)
        if (classes[i].startsWith('pin-')) {
          this.active_pin = classes[i].substr(4)

          if (this.active != undefined)
            this.active.classList.remove('selected-pin')
          this.active = e.currentTarget
          classes.add('selected-pin')
        }
    })
  }
}
