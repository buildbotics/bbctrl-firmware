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

#include "button.h"
#include "util.h"

#include <X11/Xatom.h>


void button_draw(Button *btn) {
  drw_rect(btn->drw, 0, 0, 100, 100, 1, 1);

  const char *label = "⌨";
  int h = btn->drw->fonts[0].xfont->height * 2;
  int y = (btn->h - h) / 2;
  int w = drw_fontset_getwidth(btn->drw, label);
  int x = (btn->w - w) / 2;
  drw_text(btn->drw, x, y, w, h, 0, label, 0);

  drw_map(btn->drw, btn->win, 0, 0, 100, 100);
}


void button_event(Button *btn, XEvent *e) {
  switch (e->type) {
  case MotionNotify: {
    int x = e->xmotion.x;
    int y = e->xmotion.y;
    btn->mouse_in = 0 <= x && x < btn->w && 0 <= y && y < btn->h;
    break;
  }

  case ButtonPress: break;

  case ButtonRelease:
    if (e->xbutton.button == 1 && btn->mouse_in && btn->kbd)
      keyboard_toggle(btn->kbd);
    break;

  case Expose: if (!e->xexpose.count) button_draw(btn); break;
  }
}


Button *button_create(Display *dpy, Keyboard *kbd, int x, int y, int w, int h,
                      const char *font) {
  Button *btn = (Button *)calloc(1, sizeof(Button));
  btn->kbd = kbd;

  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  // Dimensions
  Dim dim = get_display_dims(dpy, screen);
  x = x < 0 ? dim.width - w : 0;
  y = y < 0 ? dim.height - h : 0;
  btn->w = w;
  btn->h = h;

  // Create drawable
  Drw *drw = btn->drw = drw_create(dpy, screen, root, w, h);

  // Setup font
  if (!drw_fontset_create(drw, &font, 1)) die("no fonts could be loaded");

  // Init color scheme
  const char *colors[] = {"#bbbbbb", "#132a33"};
  btn->scheme = drw_scm_create(drw, colors, 2);
  drw_setscheme(drw, btn->scheme);

  XSetWindowAttributes wa;
  wa.override_redirect = true;

  btn->win = XCreateWindow
    (dpy, root, x, y, w, h, 0, CopyFromParent, CopyFromParent,
     CopyFromParent, CWOverrideRedirect | CWBorderPixel | CWBackingPixel, &wa);

  // Enable window events
  XSelectInput(dpy, btn->win, ButtonReleaseMask | ButtonPressMask |
               ExposureMask | PointerMotionMask);

  // Set window properties
  XWMHints *wmHints = XAllocWMHints();
  wmHints->input = false;
  wmHints->flags = InputHint;

  const char *name = "bbkbd-button";
  XTextProperty str;
  XStringListToTextProperty((char **)&name, 1, &str);

  XClassHint *classHints = XAllocClassHint();
  classHints->res_class = (char *)name;
  classHints->res_name = (char *)name;

  XSetWMProperties(dpy, btn->win, &str, &str, 0, 0, 0, wmHints, classHints);

  XFree(classHints);
  XFree(wmHints);
  XFree(str.value);

  // Set window type
  Atom atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", false);
  Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", false);
  XChangeProperty(dpy, btn->win, atom, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&type, 1);

  // Raise window to top of stack
  XMapRaised(dpy, btn->win);

  return btn;
}


void button_destroy(Button *btn) {
  drw_sync(btn->drw);
  drw_free(btn->drw);

  free(btn->scheme);
  free(btn);
}
