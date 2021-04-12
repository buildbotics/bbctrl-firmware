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

#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif


bool debug = false;


void die(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);

  } else fputc('\n', stderr);

  exit(1);
}


void print_dbg(const char *fmt, ...) {
  if (!debug) return;

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fflush(stderr);
}



int find_unused_keycode(Display *dpy) {
  // Derived from:
  // https://stackoverflow.com/questions/44313966/
  //   c-xtest-emitting-key-presses-for-every-unicode-character

  int keycode_low, keycode_high;
  XDisplayKeycodes(dpy, &keycode_low, &keycode_high);

  int keysyms_per_keycode = 0;
  KeySym *keysyms =
    XGetKeyboardMapping(dpy, keycode_low, keycode_high - keycode_low,
                        &keysyms_per_keycode);

  for (int i = keycode_low; i <= keycode_high; i++) {
    bool key_is_empty = true;

    for (int j = 0; j < keysyms_per_keycode; j++) {
      int symindex = (i - keycode_low) * keysyms_per_keycode + j;
      if (keysyms[symindex]) key_is_empty = false;
      else break;
    }

    if (key_is_empty) {
      XFree(keysyms);
      return i;
    }
  }

  XFree(keysyms);
  return 1;
}


void simulate_key(Display *dpy, KeySym keysym, bool press) {
  if (!keysym) return;

  KeyCode code = XKeysymToKeycode(dpy, keysym);

  if (!code) {
    static int tmp_keycode = 0;
    if (!tmp_keycode) tmp_keycode = find_unused_keycode(dpy);

    code = tmp_keycode;
    XChangeKeyboardMapping(dpy, tmp_keycode, 1, &keysym, 1);
    XSync(dpy, false);
  }

  XTestFakeKeyEvent(dpy, code, press, 0);
}


Dim get_display_dims(Display *dpy, int screen) {
  Dim dim;

#ifdef XINERAMA
  if (XineramaIsActive(dpy)) {
    int i = 0;
    XineramaScreenInfo *info = XineramaQueryScreens(dpy, &i);
    dim.width  = info[0].width;
    dim.height = info[0].height;
    XFree(info);
    return dim;
  }
#endif

  dim.width  = DisplayWidth(dpy, screen);
  dim.height = DisplayHeight(dpy, screen);

  return dim;
}
