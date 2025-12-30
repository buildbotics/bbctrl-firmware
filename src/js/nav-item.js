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



module.exports = {
  template: '<div class="nav-item", @mouseenter="show", @click.stop="toggle">' +
    '<slot></slot></div>',

  ready: function() {
    // Close menu when clicking outside
    var self = this
    $(document).on('click.navitem', function(e) {
      if (!$(e.target).closest('.nav-item').length) {
        $('.nav-menu-open').removeClass('nav-menu-open')
      }
    })
  },
  
  beforeDestroy: function() {
    $(document).off('click.navitem')
  },

  methods: {
    show(e) {
      $(e.currentTarget).find('.nav-menu-hide').removeClass('nav-menu-hide')
    },
    
    toggle(e) {
      var menu = $(e.currentTarget).find('.nav-menu')
      // Toggle using nav-menu-open class for click behavior
      if (menu.hasClass('nav-menu-open')) {
        menu.removeClass('nav-menu-open')
      } else {
        // Close any other open menus first
        $('.nav-menu-open').removeClass('nav-menu-open')
        menu.addClass('nav-menu-open')
      }
    }
  }
}
