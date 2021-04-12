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

#pragma once

#include "keyboard.h"


static const char *colors[SchemeLast][2] = {
  //                         fg         bg
  [SchemeNorm]           = {"#bbbbbb", "#132a33"},
  [SchemeNormABC]        = {"#ffffff", "#14313d"},
  [SchemePress]          = {"#ffffff", "#259937"},
  [SchemeHighlight]      = {"#58a7c6", "#005577"},
};


static Key _main[] = {
  {"q", "Q", XK_q, 1},
  {"w", "W", XK_w, 1},
  {"e", "E", XK_e, 1},
  {"r", "R", XK_r, 1},
  {"t", "T", XK_t, 1},
  {"y", "Y", XK_y, 1},
  {"u", "U", XK_u, 1},
  {"i", "I", XK_i, 1},
  {"o", "O", XK_o, 1},
  {"p", "P", XK_p, 1},
  {"7", "&", XK_7, 1},
  {"8", "*", XK_8, 1},
  {"9", "(", XK_9, 1},
  {"-", "_", XK_minus, 1},

  {0}, // New row

  {"a", "A", XK_a, 1},
  {"s", "S", XK_s, 1},
  {"d", "D", XK_d, 1},
  {"f", "F", XK_f, 1},
  {"g", "G", XK_g, 1},
  {"h", "H", XK_h, 1},
  {"j", "J", XK_j, 1},
  {"k", "K", XK_k, 1},
  {"l", "L", XK_l, 1},
  {";", ":", XK_colon, 1},
  {"4", "$", XK_4, 1},
  {"5", "%", XK_5, 1},
  {"6", "^", XK_6, 1},
  {"=", "+", XK_equal, 1},

  {0}, // New row

  {"z", "Z", XK_z, 1},
  {"x", "X", XK_x, 1},
  {"c", "C", XK_c, 1},
  {"v", "V", XK_v, 1},
  {"b", "B", XK_b, 1},
  {"n", "N", XK_n, 1},
  {"m", "M", XK_m, 1},
  {"Tab", 0, XK_Tab, 1},
  {"⇍ Bksp", 0, XK_BackSpace, 2},
  {"1", "!", XK_1, 1},
  {"2", "@", XK_2, 1},
  {"3", "#", XK_3, 1},
  {"/", "?", XK_slash, 1},

  {0}, // New row
  {"⌨", 0, XK_Cancel, 1},
  {"Shift", 0, XK_Shift_L, 1},
  {"↓", 0, XK_Down, 1},
  {"↑", 0, XK_Up, 1},
  {"Space", 0, XK_space, 2},
  {"Esc", 0, XK_Escape, 1},
  {"Ctrl", 0, XK_Control_L, 1},
  {"↲ Enter", 0, XK_Return, 2},
  {"0", ")", XK_0, 1},
  {",", "<", XK_comma, 1},
  {".", ">", XK_period, 1},
  {"\\", "|", XK_slash, 1},

  {0}, {0} // End
};


static Key _alt[] = {
  {0, 0, XK_Q, 1},
  {0, 0, XK_W, 1},
  {0, 0, XK_E, 1},
  {0, 0, XK_R, 1},
  {0, 0, XK_T, 1},
  {0, 0, XK_Y, 1},
  {0, 0, XK_U, 1},
  {0, 0, XK_I, 1},
  {0, 0, XK_O, 1},
  {0, 0, XK_P, 1},
  {"7", 0, XK_7, 1},
  {"8", 0, XK_8, 1},
  {"9", 0, XK_9, 1},
  {"-", 0, XK_minus, 1},

  {0}, // New row

  {0, 0, XK_A, 1},
  {0, 0, XK_S, 1},
  {0, 0, XK_D, 1},
  {0, 0, XK_F, 1},
  {0, 0, XK_G, 1},
  {0, 0, XK_H, 1},
  {0, 0, XK_J, 1},
  {0, 0, XK_K, 1},
  {0, 0, XK_L, 1},
  {";",":", XK_colon, 1},
  {"4", 0, XK_4, 1},
  {"5", 0, XK_5, 1},
  {"6", 0, XK_6, 1},
  {"+", 0, XK_plus, 1},

  {0}, // New row

  {0, 0, XK_Z, 1},
  {0, 0, XK_X, 1},
  {0, 0, XK_C, 1},
  {0, 0, XK_V, 1},
  {0, 0, XK_B, 1},
  {0, 0, XK_N, 1},
  {0, 0, XK_M, 1},
  {"Tab", 0, XK_Tab, 1},
  {"⇍ Bksp", 0, XK_BackSpace, 2},
  {"1", 0, XK_1, 1},
  {"2", 0, XK_2, 1},
  {"3", 0, XK_3, 1},
  {"/", 0, XK_slash, 1},

  {0}, // New row
  {"⌨", 0, XK_Cancel, 1},
  {"Shift", 0, XK_Shift_L, 1},
  {"↓", 0, XK_Down, 1},
  {"↑", 0, XK_Up, 1},
  {"Space", 0, XK_space, 2},
  {"Esc", 0, XK_Escape, 1},
  {"Ctrl", 0, XK_Control_L, 1},
  {"↲ Enter", 0, XK_Return, 2},
  {"0", 0, XK_0, 1},
  {".", 0, XK_period, 1},
  {"=", 0, XK_equal, 1},
  {"*", 0, XK_asterisk, 1},

  {0}, {0} // End
};


Key _symbols[] = {
  {"1", "!", XK_1, 1},
  {"2", "@", XK_2, 1},
  {"3", "#", XK_3, 1},
  {"4", "$", XK_4, 1},
  {"5", "%", XK_5, 1},
  {"6", "^", XK_6, 1},
  {"7", "&", XK_7, 1},
  {"8", "*", XK_8, 1},
  {"9", "(", XK_9, 1},
  {"0", ")", XK_0, 1},

  {0}, // New row

  {"'", "\"", XK_apostrophe, 1},
  {"`", "~", XK_grave, 1},
  {"-", "_", XK_minus, 1},
  {"=", "+", XK_plus, 1},
  {"[", "{", XK_bracketleft, 1},
  {"]", "}", XK_bracketright, 1},
  {",", "<", XK_comma, 1},
  {".", ">", XK_period, 1},
  {"/", "?", XK_slash, 1},
  {"\\", "|", XK_backslash, 1},

  {0}, // New row

  {"", 0, XK_Shift_L|XK_bar, 1},
  {"⇤", 0, XK_Home, 1},
  {"←", 0, XK_Left, 1},
  {"→", 0, XK_Right, 1},
  {"⇥", 0, XK_End, 1},
  {"⇊", 0, XK_Next, 1},
  {"⇈", 0, XK_Prior, 1},
  {"Tab", 0, XK_Tab, 1},
  {"⇍ Bksp", 0, XK_BackSpace, 2},

  {0}, // New row
  {"⌨", 0, XK_Cancel, 1},
  {"Shift", 0, XK_Shift_L, 1},
  {"↓", 0, XK_Down, 1},
  {"↑", 0, XK_Up, 1},
  {"", 0, XK_space, 2},
  {"Esc", 0, XK_Escape, 1},
  {"Ctrl", 0, XK_Control_L, 1},
  {"↲ Enter", 0, XK_Return, 2},

  {0}, {0} // End
};


static Key *layers[] = {
  _main,
  _alt,
  0
};
