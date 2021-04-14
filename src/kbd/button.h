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

#include <stdbool.h>

typedef void (*button_cb)();

typedef struct {
  Window win;
  Drw *drw;
  Clr *scheme;

  button_cb cb;
  void *cb_data;

  int w;
  int h;
  bool mouse_in;
} Button;


Button *button_create(Display *dpy, float x, float y, int w, int h,
                      const char *font);
void button_destroy(Button *btn);
void button_set_callback(Button *btn, button_cb cb, void *data);
void button_event(Button *btn, XEvent *e);
