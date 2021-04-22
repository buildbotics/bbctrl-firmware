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


// Must match modbus.c
var exports = {
  DISCONNECTED: 0,
  OK:           1,
  CRC:          2,
  INVALID:      3,
  TIMEDOUT:     4
};


exports.status_to_string =
  function (status) {
    if (status == exports.OK)       return 'Ok';
    if (status == exports.CRC)      return 'CRC error';
    if (status == exports.INVALID)  return 'Invalid response';
    if (status == exports.TIMEDOUT) return 'Timedout';
    return 'Disconnected';
  }


module.exports = exports;
