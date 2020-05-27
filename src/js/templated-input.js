/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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


module.exports = {
  replace: true,
  template: '#templated-input-template',
  props: ['name', 'model', 'template'],


  data: function () {return {view: ''}},


  computed: {
    metric: function () {return this.$root.metric()},


    _view: function () {
      if (this.template.scale) {
        if (this.metric) return 1 * this.model.toFixed(3);

        return 1 * (this.model / this.template.scale).toFixed(4);
      }

      return this.model;
    },


    units: function () {
      return (this.metric || !this.template.iunit) ?
        this.template.unit : this.template.iunit;
    },


    title: function () {
      var s = 'Default ' + this.template.default + ' ' +
          (this.template.unit || '');
      if (typeof this.template.help != 'undefined')
        s = this.template.help + '\n' + s;
      return s;
    }
  },


  watch: {
    _view: function () {this.view = this._view},


    view: function () {
      if (this.template.scale && !this.metric)
        this.model = this.view * this.template.scale;
      else this.model = this.view;
    }
  },


  ready: function () {this.view = this._view},


  methods: {
    change: function () {this.$dispatch('input-changed')}
  }
}
