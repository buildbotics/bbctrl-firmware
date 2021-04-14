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
  //                    fg         bg
  [SchemeNorm]       = {"#bbbbbb", "#272a2b"},
  [SchemeNormABC]    = {"#ffffff", "#383c3d"},
  [SchemePress]      = {"#ffffff", "#e5aa3d"},
  [SchemeHighlight]  = {"#bbbbbb", "#666666"},
  [SchemeBG]         = {"#ffffff", "#000000"},
};


static Key row0[] = {
  {"`", "~", XK_grave, 1},
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
  {"-", "_", XK_minus, 1},
  {"=", "+", XK_equal, 1},
  {"Back", 0, XK_BackSpace, 1},
  {0}
};

static Key row1[] = {
  {"Tab ➡", "Tab ⬅", XK_Tab, 1},
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
  {"[", "{", XK_bracketleft, 1},
  {"]", "}", XK_bracketright, 1},
  {"\\", "|", XK_backslash, 1},
  {0}
};

static Key row2[] = {
  {"Esc", 0, XK_Escape, 1},
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
  {"\"", "'", XK_quotedbl, 1},
  {"↲ Enter", 0, XK_Return, 2},
  {0}
};

static Key row3[] = {
  {"⬆ Shift", 0, XK_Shift_L, 2},
  {"z", "Z", XK_z, 1},
  {"x", "X", XK_x, 1},
  {"c", "C", XK_c, 1},
  {"v", "V", XK_v, 1},
  {"b", "B", XK_b, 1},
  {"n", "N", XK_n, 1},
  {"m", "M", XK_m, 1},
  {",", "<", XK_comma, 1},
  {".", ">", XK_period, 1},
  {"/", "?", XK_slash, 1},
  {"⬆ Shift", 0, XK_Shift_L, 2},
  {0}
};

static Key row4[] = {
  {"Ctrl", 0, XK_Control_L, 2},
  {"Space", 0, XK_space, 10},
  {"Alt", 0, XK_Alt_R, 2},
  {0}
};

static Key *keys[] = {row0, row1, row2, row3, row4, 0};
