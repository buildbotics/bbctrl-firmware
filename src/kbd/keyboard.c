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

#include "keyboard.h"

#include <X11/Xatom.h>

#include <signal.h>


static int create_window(Display *dpy, int root, const char *name, Dim dim,
                         int x, int y, bool override, unsigned long fg,
                         unsigned long bg) {
  XSetWindowAttributes wa;
  wa.override_redirect = override;
  wa.border_pixel = fg;
  wa.background_pixel = bg;

  int win = XCreateWindow
    (dpy, root, x, y, dim.width, dim.height, 0, CopyFromParent, CopyFromParent,
     CopyFromParent, CWOverrideRedirect | CWBorderPixel | CWBackingPixel, &wa);

  // Enable window events
  XSelectInput(dpy, win, StructureNotifyMask | ButtonReleaseMask |
               ButtonPressMask | ExposureMask | PointerMotionMask |
               LeaveWindowMask);

  // Set window properties
  XWMHints *wmHints = XAllocWMHints();
  wmHints->input = false;
  wmHints->flags = InputHint;

  XTextProperty str;
  XStringListToTextProperty((char **)&name, 1, &str);

  XClassHint *classHints = XAllocClassHint();
  classHints->res_class = (char *)name;
  classHints->res_name = (char *)name;

  XSetWMProperties(dpy, win, &str, &str, 0, 0, 0, wmHints, classHints);

  XFree(classHints);
  XFree(wmHints);
  XFree(str.value);

  // Set window type
  Atom atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", false);
  Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", false);
  XChangeProperty(dpy, win, atom, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&type, 1);

  return win;
}


static bool is_modifier(Key *k) {return k && IsModifierKey(k->keysym);}


static int key_scheme(Keyboard *kbd, Key *k) {
  if (k->pressed) return SchemePress;
  if (k == kbd->focus) return SchemeHighlight;
  if (k->keysym == XK_Return || (XK_a <= k->keysym && k->keysym <= XK_z) ||
      (XK_Cyrillic_io <= k->keysym && k->keysym <= XK_Cyrillic_hardsign))
    return SchemeNormABC;

  return SchemeNorm;
}


Key *keyboard_find_key(Keyboard *kbd, int x, int y) {
  Key *keys = kbd->keys;

  for (int i = 0; i < kbd->nkeys; i++)
    if (keys[i].keysym && keys[i].x < x && x < keys[i].x + keys[i].w &&
        keys[i].y < y && y < keys[i].y + keys[i].h)
      return &keys[i];

  return 0;
}


void keyboard_draw_key(Keyboard *kbd, Key *k) {
  Drw *drw = kbd->drw;

  drw_setscheme(drw, kbd->scheme[key_scheme(kbd, k)]);
  drw_rect(drw, k->x, k->y, k->w, k->h, 1, 1);

  const char *label = k->label;
  if (!label) label = XKeysymToString(k->keysym);
  if (kbd->shifted && k->label2) label = k->label2;

  int h = drw->fonts[0].xfont->height * 2;
  int y = k->y + (k->h - h) / 2;
  int w = drw_fontset_getwidth(drw, label);
  int x = k->x + (k->w - w) / 2;
  drw_text(drw, x, y, w, h, 0, label, 0);

  drw_map(drw, kbd->win, k->x, k->y, k->w, k->h);
}


void keyboard_draw(Keyboard *kbd) {
  for (int i = 0; i < kbd->nkeys; i++)
    if (kbd->keys[i].keysym)
      keyboard_draw_key(kbd, &kbd->keys[i]);
}


void keyboard_update(Keyboard *kbd) {
  int y = 0;
  int r = kbd->nrows;
  int h = (kbd->dim.height - 1) / r;
  Key *keys = kbd->keys;

  for (int i = 0; i < kbd->nkeys; i++, r--) {
    int base = 0;

    for (int j = i; j < kbd->nkeys && keys[j].keysym; j++)
      base += keys[j].width;

    for (int x = 0; i < kbd->nkeys && keys[i].keysym; i++) {
      keys[i].x = x;
      keys[i].y = y;
      keys[i].w = keys[i].width * (kbd->dim.width - 1) / base;
      keys[i].h = r == 1 ? kbd->dim.height - y - 1 : h;
      x += keys[i].w;
    }

    if (base) keys[i - 1].w = kbd->dim.width - 1 - keys[i - 1].x;
    y += h;
  }

  keyboard_draw(kbd);
}


void keyboard_init_layer(Keyboard *kbd) {
  Key *layer = kbd->layers[kbd->layer];

  // Count keys
  kbd->nkeys = 0;

  for (int i = 0; ; i++) {
    if (0 < i && !layer[i].keysym && !layer[i - 1].keysym) {
      kbd->nkeys--;
      break;
    }

    kbd->nkeys++;
  }

  kbd->keys = calloc(1, sizeof(Key) * kbd->nkeys);
  memcpy(kbd->keys, layer, sizeof(Key) * kbd->nkeys);

  // Count rows
  kbd->nrows = 1;

  for (int i = 0; i < kbd->nkeys; i++)
    if (!kbd->keys[i].keysym) {
      kbd->nrows++;

      if (i && !kbd->keys[i - 1].keysym) {
        kbd->nrows--;
        break;
      }
    }
}


void keyboard_next_layer(Keyboard *kbd) {
  if (!kbd->layers[++kbd->layer]) kbd->layer = 0;

  print_dbg("Cycling to layer %d\n", kbd->layer);

  keyboard_init_layer(kbd);
  keyboard_update(kbd);
}


void keyboard_press_key(Keyboard *kbd, Key *k) {
  if (k->pressed) return;

  if (k->keysym == XK_Shift_L || k->keysym == XK_Shift_R) {
    kbd->shifted = true;
    keyboard_draw(kbd);
  }

  simulate_key(kbd->drw->dpy, k->modifier, true);
  simulate_key(kbd->drw->dpy, k->keysym, true);
  k->pressed = true;
  keyboard_draw_key(kbd, k);
}


void keyboard_unpress_key(Keyboard *kbd, Key *k) {
  if (!k->pressed) return;

  if (k->keysym == XK_Shift_L || k->keysym == XK_Shift_R) {
    kbd->shifted = false;
    keyboard_draw(kbd);
  }

  simulate_key(kbd->drw->dpy, k->keysym, false);
  simulate_key(kbd->drw->dpy, k->modifier, false);
  k->pressed = false;
  keyboard_draw_key(kbd, k);
}


void keyboard_unpress_all(Keyboard *kbd) {
  for (int i = 0; i < kbd->nkeys; i++)
    keyboard_unpress_key(kbd, &kbd->keys[i]);
}


void keyboard_mouse_motion(Keyboard *kbd, int x, int y) {
  Key *k = keyboard_find_key(kbd, x, y);
  Key *focus = kbd->focus;

  if (k == focus) return;
  kbd->focus = k;

  if (focus && !is_modifier(focus))
    keyboard_unpress_key(kbd, focus);

  if (k && kbd->is_pressing && !is_modifier(k))
    keyboard_press_key(kbd, k);

  if (k) keyboard_draw_key(kbd, k);
  if (focus) keyboard_draw_key(kbd, focus);
}


void keyboard_mouse_press(Keyboard *kbd, int x, int y) {
  kbd->is_pressing = true;

  Key *k = keyboard_find_key(kbd, x, y);
  if (k) {
    if (is_modifier(k) && k->pressed) keyboard_unpress_key(kbd, k);
    else keyboard_press_key(kbd, k);
  }
}


void keyboard_mouse_release(Keyboard *kbd, int x, int y) {
  kbd->is_pressing = false;

  Key *k = keyboard_find_key(kbd, x, y);
  if (k) {
    switch (k->keysym) {
    case XK_Cancel: keyboard_next_layer(kbd); break;
    case XK_Break: raise(SIGINT); break;
    default: break;
    }

    if (!is_modifier(k)) keyboard_unpress_all(kbd);
  }
}


void keyboard_resize(Keyboard *kbd, int width, int height) {
  if (width == kbd->dim.width && height == kbd->dim.height) return;

  kbd->dim.width = width;
  kbd->dim.height = height;
  drw_resize(kbd->drw, width, height);
  keyboard_update(kbd);
}


void keyboard_event(Keyboard *kbd, XEvent *e) {
  switch (e->type) {
  case LeaveNotify: keyboard_mouse_motion(kbd, -1, -1); break;

  case MotionNotify:
    keyboard_mouse_motion(kbd, e->xmotion.x, e->xmotion.y);
    break;

  case ButtonPress:
    if (e->xbutton.button == 1)
      keyboard_mouse_press(kbd, e->xbutton.x, e->xbutton.y);
    break;

  case ButtonRelease:
    if (e->xbutton.button == 1)
      keyboard_mouse_release(kbd, e->xbutton.x, e->xbutton.y);
    break;

  case ConfigureNotify:
    keyboard_resize(kbd, e->xconfigure.width, e->xconfigure.height);
    break;

  case Expose:
    if (!e->xexpose.count) keyboard_draw(kbd);
    break;
  }
}


void keyboard_toggle(Keyboard *kbd) {
  kbd->visible = !kbd->visible;

  if (kbd->visible) XMapRaised(kbd->drw->dpy, kbd->win);
  else {
    XUnmapWindow(kbd->drw->dpy, kbd->win);
    keyboard_unpress_all(kbd);
  }
}


Keyboard *keyboard_create(Display *dpy, Key **layers, const char *font,
                          const char *colors[SchemeLast][2]) {
  Keyboard *kbd = calloc(1, sizeof(Keyboard));
  kbd->layers = layers;

  // Init screen
  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  // Get display size
  Dim dim = get_display_dims(dpy, screen);

  // Init keyboard layer
  keyboard_init_layer(kbd); // Computes kbd->nrows
  kbd->dim.width = dim.width;
  kbd->dim.height = dim.height * kbd->nrows / 18;

  // Create drawable
  Drw *drw = kbd->drw =
    drw_create(dpy, screen, root, kbd->dim.width, kbd->dim.height);

  // Setup fonts
  if (!drw_fontset_create(drw, &font, 1)) die("no fonts could be loaded");

  // Init color schemes
  for (int i = 0; i < SchemeLast; i++)
    kbd->scheme[i] = drw_scm_create(drw, colors[i], 2);

  drw_setscheme(drw, kbd->scheme[SchemeNorm]);

  // Create window
  int y = dim.height - kbd->dim.height;
  Clr *clr = kbd->scheme[SchemeNorm];
  kbd->win = create_window(dpy, root, "bbkbd", kbd->dim, 0, y, false,
                           clr[ColFg].pixel, clr[ColBg].pixel);

  // Init keyboard
  keyboard_update(kbd);

  return kbd;
}


void keyboard_destroy(Keyboard *kbd) {
  Display *dpy = kbd->drw->dpy;

  keyboard_unpress_all(kbd);

  drw_sync(kbd->drw);
  drw_free(kbd->drw);

  for (int i = 0; i < SchemeLast; i++)
    free(kbd->scheme[i]);

  XSync(dpy, false);
  XDestroyWindow(dpy, kbd->win);
  XSync(dpy, false);
  XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);

  free(kbd->keys);
  free(kbd);
}
