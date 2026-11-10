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


// Bootstrap Icons (MIT, https://icons.getbootstrap.com), drawn on a 16x16
// grid in the current text colour.  Each icon is a list of path outlines.
const icons = {
  'zero-square-fill': [
    'M8 4.951c-1.008 0-1.629 1.09-1.629 2.895v.31c0 1.81.627 2.895 1.629 ' +
      '2.895s1.623-1.09 1.623-2.895v-.31c0-1.8-.621-2.895-1.623-2.895',
    'M2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 ' +
      '0-2-2zm5.988 12.158c-1.851 0-2.941-1.57-2.941-3.99V7.84c0-2.408 ' +
      '1.101-3.996 2.965-3.996 1.857 0 2.935 1.57 2.935 3.996v.328c0 ' +
      '2.408-1.101 3.99-2.959 3.99'
  ],

  'house-door-fill': [
    'M6.5 14.5v-3.505c0-.245.25-.495.5-.495h2c.25 0 .5.25.5.5v3.5a.5.5 0 0 0 ' +
      '.5.5h4a.5.5 0 0 0 .5-.5v-7a.5.5 0 0 0-.146-.354L13 5.793V2.5a.5.5 0 ' +
      '0 0-.5-.5h-1a.5.5 0 0 0-.5.5v1.293L8.354 1.146a.5.5 0 0 0-.708 0l-6 ' +
      '6A.5.5 0 0 0 1.5 7.5v7a.5.5 0 0 0 .5.5h4a.5.5 0 0 0 .5-.5'
  ],

  'check-square-fill': [
    'M2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 ' +
      '0-2-2zm10.03 4.97a.75.75 0 0 1 .011 1.05l-3.992 4.99a.75.75 0 0 ' +
      '1-1.08.02L4.324 8.384a.75.75 0 1 1 1.06-1.06l2.094 2.093 ' +
      '3.473-4.425a.75.75 0 0 1 1.08-.022z'
  ],

  'question-square-fill': [
    'M2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 ' +
      '0-2-2zm3.496 6.033a.237.237 0 0 1-.24-.247C5.35 4.091 6.737 3.5 ' +
      '8.005 3.5c1.396 0 2.672.73 2.672 2.24 0 1.08-.635 1.594-1.244 ' +
      '2.057-.737.559-1.01.768-1.01 1.486v.105a.25.25 0 0 1-.25.25h-.81a.25' +
      '.25 0 0 1-.25-.246l-.004-.217c-.038-.927.495-1.498 1.168-1.987.59-.444' +
      '.965-.736.965-1.371 0-.825-.628-1.168-1.314-1.168-.803 0-1.253.478-' +
      '1.342 1.134-.018.137-.128.25-.266.25h-.825zm2.325 6.443c-.584 ' +
      '0-1.009-.394-1.009-.927 0-.552.425-.94 1.01-.94.609 0 1.028.388 ' +
      '1.028.94 0 .533-.42.927-1.029.927'
  ],

  'exclamation-square-fill': [
    'M0 2a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 ' +
      '1-2-2zm8 4c-.535 0-.954.462-.9.995l.35 3.507a.552.552 0 0 0 1.1 ' +
      '0l.35-3.507A.905.905 0 0 0 8 4m.002 6a1 1 0 1 0 0 2 1 1 0 0 0 0-2'
  ],

  'box': [
    'M8.186 1.113a.5.5 0 0 0-.372 0L1.846 3.5 8 5.961 14.154 3.5zM15 ' +
      '4.239l-6.5 2.6v7.922l6.5-2.6V4.24zM7.5 14.762V6.838L1 4.239v7.923zM7.443' +
      '.184a1.5 1.5 0 0 1 1.114 0l7.129 2.852A.5.5 0 0 1 16 3.5v8.662a1 1 0 0 ' +
      '1-.629.928l-7.185 2.874a.5.5 0 0 1-.372 0L.63 13.09a1 1 0 0 ' +
      '1-.63-.928V3.5a.5.5 0 0 1 .314-.464z'
  ],

  'camera-video': [
    'M0 5a2 2 0 0 1 2-2h7.5a2 2 0 0 1 1.983 1.738l3.11-1.382A1 1 0 0 1 16 ' +
      '4.269v7.462a1 1 0 0 1-1.406.913l-3.111-1.382A2 2 0 0 1 9.5 13H2a2 2 0 0 ' +
      '1-2-2zm11.5 5.175 3.5 1.556V4.269l-3.5 1.556zM2 4a1 1 0 0 0-1 1v6a1 1 0 ' +
      '0 0 1 1h7.5a1 1 0 0 0 1-1V5a1 1 0 0 0-1-1z'
  ],

  'chat-left-text': [
    'M14 1a1 1 0 0 1 1 1v8a1 1 0 0 1-1 1H4.414A2 2 0 0 0 3 11.586l-2 2V2a1 1 ' +
      '0 0 1 1-1zM2 0a2 2 0 0 0-2 2v12.793a.5.5 0 0 0 .854.353l2.853-2.853A1 1 ' +
      '0 0 1 4.414 12H14a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2z',
    'M3 3.5a.5.5 0 0 1 .5-.5h9a.5.5 0 0 1 0 1h-9a.5.5 0 0 1-.5-.5M3 6a.5.5 0 0 ' +
      '1 .5-.5h9a.5.5 0 0 1 0 1h-9A.5.5 0 0 1 3 6m0 2.5a.5.5 0 0 1 .5-.5h5a.5.5 ' +
      '0 0 1 0 1h-5a.5.5 0 0 1-.5-.5'
  ],

  'arrow-clockwise': [
    'M8 3a5 5 0 1 0 4.546 2.914.5.5 0 0 1 .908-.417A6 6 0 1 1 8 2z',
    'M8 4.466V.534a.25.25 0 0 1 .41-.192l2.36 1.966c.12.1.12.284 0 .384L8.41 ' +
      '4.658A.25.25 0 0 1 8 4.466'
  ],

  'arrow-counterclockwise': [
    'M8 3a5 5 0 1 1-4.546 2.914.5.5 0 0 0-.908-.417A6 6 0 1 0 8 2z',
    'M8 4.466V.534a.25.25 0 0 0-.41-.192L5.23 2.308a.25.25 0 0 0 0 .384l2.36 ' +
      '1.966A.25.25 0 0 0 8 4.466'
  ],

  'x-square-fill': [
    'M2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 ' +
      '0-2-2zm3.354 4.646L8 7.293l2.646-2.647a.5.5 0 0 1 .708.708L8.707 ' +
      '8l2.647 2.646a.5.5 0 0 1-.708.708L8 8.707l-2.646 2.647a.5.5 0 0 ' +
      '1-.708-.708L7.293 8 4.646 5.354a.5.5 0 1 1 .708-.708'
  ],
}


// Icons whose source SVG sets fill-rule="evenodd" to cut out their holes
const evenodd = [
  'camera-video', 'arrow-clockwise', 'arrow-counterclockwise'
]


module.exports = {
  template: '#bi-icon-template',
  props: ['name'],


  computed: {
    paths() {return icons[this.name] || []},
    fill_rule() {return evenodd.includes(this.name) ? 'evenodd' : 'nonzero'}
  }
}
