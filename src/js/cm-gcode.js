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

'use strict';


CodeMirror.defineMode('gcode', function (config, parserConfig) {
  return {
    token: function (stream, state) {
      if (stream.eatSpace()) return null;

      if (stream.match(';')) {
        stream.skipToEnd();
        return 'comment';
      }

      if (stream.match('(')) {
        if (stream.skipTo(')')) stream.next();
        else stream.skipToEnd();
        return 'comment';
      }

      if (stream.match(/[+-]?[\d.]+/))     return 'number';
      if (stream.match(/[\/*%=+-]/))       return 'operator';
      if (stream.match('[\[\]]'))          return 'bracket';
      if (stream.match(/N\d+/i))           return 'line';
      if (stream.match(/O\d+\s*[a-z]+/i))  return 'ocode';
      if (stream.match(/[F][+-]?[\d.]+/i)) return 'feed';
      if (stream.match(/[S][+-]?[\d.]+/i)) return 'speed';
      if (stream.match(/[T]\d+/i))         return 'tool';
      if (stream.match(/[GM][\d.]+/i))     return 'gcode';
      if (stream.match(/[A-Z]/i))          return 'id';
      if (stream.match(/#<[_a-z\d]+>/i))   return 'variable';
      if (stream.match(/#\d+/))            return 'ref';

      stream.next();
      return 'error';
    }
  }
})
