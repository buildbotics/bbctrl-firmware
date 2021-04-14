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

#include "drw.h"
#include "util.h"

#include <stdbool.h>


enum {
  SchemeNorm, SchemeNormABC, SchemePress, SchemeHighlight, SchemeBG, SchemeLast
};

typedef void (*keyboard_show_cb)(bool show);

typedef struct {
  char *label;
  char *label2;
  KeySym keysym;
  unsigned width;
  int x, y, w, h;
  bool pressed;
} Key;

typedef struct {
  Window win;
  Drw *drw;

  int space;
  int w, h;
  int x, y;
  int rows;
  int cols;

  bool meta;
  bool shift;
  bool visible;

  Key *pressed;
  Key *focus;
  Key **keys;

  char *font;
  Clr *scheme[SchemeLast];

  keyboard_show_cb show_cb;
} Keyboard;


void keyboard_destroy(Keyboard *kbd);
Keyboard *keyboard_create(Display *dpy, Key **keys, int space, const char *font,
                          const char *colors[SchemeLast][2]);

void keyboard_event(Keyboard *kbd, XEvent *e);
void keyboard_toggle(Keyboard *kbd);
