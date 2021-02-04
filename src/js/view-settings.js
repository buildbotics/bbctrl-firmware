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


var api = require('./api');


module.exports = {
  template: '#view-settings-template',
  props: ['config', 'template', 'state'],


  data: function () {
    return {
      index: -1,
      view: undefined,
      modified: false
    }
  },


  components: {
    'settings-not-found': {template: '<h2>Settings page not found</h2>'},
    'settings-general':   require('./settings-general'),
    'settings-motor':     require('./settings-motor'),
    'settings-tool':      require('./settings-tool'),
    'settings-io':        require('./settings-io'),
    'settings-macros':    require('./settings-macros'),
    'settings-network':   require('./settings-network'),
    'settings-admin':     require('./settings-admin')
  },


  events: {
    'config-changed': function () {
      this.modified = true;
      return false;
    },


    'input-changed': function() {
      this.$dispatch('config-changed');
      return false;
    }
  },


  ready: function () {
    $(window).on('hashchange', this.parse_hash);
    this.parse_hash();
  },


  methods: {
    parse_hash: function () {
      var hash = location.hash.substr(1);

      if (!hash.trim().length) {
        location.hash = 'settings:general';
        return;
      }

      var parts = hash.split(':');
      var view = parts.length == 1 ? 'general' : parts[1];

      if (parts.length == 3) this.index = parts[2];

      if (typeof this.$options.components['settings-' + view] == 'undefined')
        this.view = 'not-found';

      else this.view = view;
    },


    save: function () {
      api.put('config/save', this.config).done(function (data) {
        this.modified = false;
      }.bind(this)).fail(function (error) {
        this.api_error('Save failed', error);
      }.bind(this));
    }
  }
}
