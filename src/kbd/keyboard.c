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
#include <X11/Xcursor/Xcursor.h>

#include <signal.h>
#include <unistd.h>


static int create_window(Display *dpy, int root, const char *name, int w, int h,
                         int x, int y, unsigned long fg, unsigned long bg) {
  XSetWindowAttributes wa;
  wa.override_redirect = false;
  wa.border_pixel = fg;
  wa.background_pixel = bg;

  int win = XCreateWindow
    (dpy, root, x, y, w, h, 0, CopyFromParent, CopyFromParent, CopyFromParent,
     CWOverrideRedirect | CWBorderPixel | CWBackingPixel, &wa);

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

  // Set cursor
  Cursor c = XcursorLibraryLoadCursor(dpy, "hand1");
  XDefineCursor(dpy, win, c);

  return win;
}


static bool is_modifier(Key *k) {return k && IsModifierKey(k->keysym);}


static int key_scheme(Keyboard *kbd, Key *k) {
  if (k->pressed || (kbd->shift && k->keysym == XK_Shift_L) ||
      (kbd->meta && k->keysym == XK_Cancel))
    return SchemePress;

  if (k == kbd->focus) return SchemeHighlight;

  if (!is_modifier(k) && k->keysym != XK_space && k->keysym != XK_Cancel)
    return SchemeNormABC;

  return SchemeNorm;
}


Key *keyboard_find_key(Keyboard *kbd, int x, int y) {

  for (int r = 0; r < kbd->rows; r++) {
    Key *keys = kbd->keys[r];

    for (int c = 0; keys[c].keysym; c++)
      if (keys[c].x < x && x < keys[c].x + keys[c].w &&
          keys[c].y < y && y < keys[c].y + keys[c].h)
        return &keys[c];
  }

  return 0;
}


void keyboard_draw_key(Keyboard *kbd, Key *k) {
  Drw *drw = kbd->drw;

  drw_setscheme(drw, kbd->scheme[key_scheme(kbd, k)]);
  drw_rect(drw, k->x, k->y, k->w, k->h, 1, 1);

  const char *label = k->label;
  if (!label) label = XKeysymToString(k->keysym);
  if (kbd->shift && k->label2) label = k->label2;

  int h = drw->fonts[0].xfont->height * 2;
  int y = k->y + (k->h - h) / 2;
  int w = drw_fontset_getwidth(drw, label);
  int x = k->x + (k->w - w) / 2;
  drw_text(drw, x, y, w, h, 0, label, 0);

  drw_map(drw, kbd->win, k->x, k->y, k->w, k->h);
}


void keyboard_draw(Keyboard *kbd) {
  drw_setscheme(kbd->drw, kbd->scheme[SchemeBG]);
  drw_rect(kbd->drw, 0, 0, kbd->w, kbd->h, 1, 1);
  drw_map(kbd->drw, kbd->win, 0, 0, kbd->w, kbd->h);

  for (int r = 0; r < kbd->rows; r++)
    for (int c = 0; kbd->keys[r][c].keysym; c++)
      keyboard_draw_key(kbd, &kbd->keys[r][c]);
}


void keyboard_layout(Keyboard *kbd) {
  int w = (kbd->w - kbd->space) / kbd->cols;
  int h = (kbd->h - kbd->space) / kbd->rows;
  int y = (kbd->h - h * kbd->rows + kbd->space) / 2;
  int xOffset = (kbd->w - w * kbd->cols + kbd->space) / 2;

  for (int r = 0; r < kbd->rows; r++) {
    Key *keys = kbd->keys[r];
    int x = xOffset;

    for (int c = 0; keys[c].keysym; c++) {
      keys[c].x = x;
      keys[c].y = y;
      keys[c].w = keys[c].width * w - kbd->space;
      keys[c].h = h - kbd->space;
      x += keys[c].w + kbd->space;
    }

    y += h;
  }

  keyboard_draw(kbd);
}


void keyboard_press_key(Keyboard *kbd, Key *k) {
  if (k->pressed) return;

  if (k->keysym == XK_Cancel) {
    kbd->meta = !kbd->meta;
    keyboard_draw_key(kbd, k);
    return;
  }

  if (k->keysym == XK_Shift_L) {
    kbd->shift = !kbd->shift;
    simulate_key(kbd->drw->dpy, XK_Shift_L, kbd->shift);
    keyboard_draw(kbd);
    return;
  }

  if (!is_modifier(k) && kbd->meta) {
    simulate_key(kbd->drw->dpy, XK_Super_L, true);
    usleep(100000);
    simulate_key(kbd->drw->dpy, XK_Super_L, false);
    usleep(100000);
    kbd->meta = false;
    keyboard_draw(kbd);
  }

  simulate_key(kbd->drw->dpy, k->keysym, true);
  k->pressed = true;
  keyboard_draw_key(kbd, k);
}


void keyboard_unpress_key(Keyboard *kbd, Key *k) {
  if (!k->pressed) return;

  simulate_key(kbd->drw->dpy, k->keysym, false);
  k->pressed = false;
  keyboard_draw_key(kbd, k);
}


void keyboard_unpress_all(Keyboard *kbd) {
  for (int r = 0; r < kbd->rows; r++)
    for (int c = 0; kbd->keys[r][c].keysym; c++)
      keyboard_unpress_key(kbd, &kbd->keys[r][c]);
}


void keyboard_mouse_motion(Keyboard *kbd, XMotionEvent *e) {
  Key *k = e ? keyboard_find_key(kbd, e->x, e->y) : 0;
  Key *focus = kbd->focus;

  if (k == focus) return;
  kbd->focus = k;

  if (focus && !is_modifier(focus))
    keyboard_unpress_key(kbd, focus);

  if (k && kbd->pressed && !is_modifier(k))
    keyboard_press_key(kbd, k);

  if (k) keyboard_draw_key(kbd, k);
  if (focus) keyboard_draw_key(kbd, focus);
}


void keyboard_mouse_press(Keyboard *kbd, XButtonEvent *e) {
  Key *k = keyboard_find_key(kbd, e->x, e->y);
  if (k) {
    if (is_modifier(k)) {
      if (k->pressed) keyboard_unpress_key(kbd, k);
      else keyboard_press_key(kbd, k);

    } else {
      keyboard_press_key(kbd, k);
      kbd->pressed = k;
    }
  }
}


void keyboard_mouse_release(Keyboard *kbd, XButtonEvent *e) {
  if (kbd->pressed) {
    keyboard_unpress_key(kbd, kbd->pressed);
    kbd->pressed = 0;
  }
}


void keyboard_resize(Keyboard *kbd, int width, int height) {
  if (width == kbd->w && height == kbd->h) return;

  kbd->w = width;
  kbd->h = height;
  drw_resize(kbd->drw, width, height);
  keyboard_layout(kbd);
}


void keyboard_event(Keyboard *kbd, XEvent *e) {
  switch (e->type) {
  case LeaveNotify: keyboard_mouse_motion(kbd, 0); break;

  case MotionNotify:
    keyboard_mouse_motion(kbd, &e->xmotion);
    break;

  case ButtonPress:
    if (e->xbutton.button == 1)
      keyboard_mouse_press(kbd, &e->xbutton);
    break;

  case ButtonRelease:
    if (e->xbutton.button == 1)
      keyboard_mouse_release(kbd, &e->xbutton);
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


Keyboard *keyboard_create(Display *dpy, Key **keys, int space, const char *font,
                          const char *colors[SchemeLast][2]) {
  Keyboard *kbd = calloc(1, sizeof(Keyboard));
  kbd->space = space;

  // Count rows & colums
  for (; keys[kbd->rows]; kbd->rows++) {
    Key *row = keys[kbd->rows];

    int cols;
    for (cols = 0; row[cols].keysym; cols++) continue;
    if (kbd->cols < cols) kbd->cols = cols;
  }

  kbd->keys = calloc(kbd->rows, sizeof(Key *));

  // Copy keys
  for (int r = 0; r < kbd->rows; r++) {
    kbd->keys[r] = calloc(kbd->cols + 1, sizeof(Key));

    for (int c = 0; keys[r][c].keysym; c++)
      kbd->keys[r][c] = keys[r][c];
  }

  // Init screen
  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  // Dimensions
  Dim dim = get_display_dims(dpy, screen);
  kbd->w = dim.width;
  kbd->h = kbd->rows * 50;
  kbd->x = 0;
  kbd->y = dim.height - kbd->h;

  // Create drawable
  Drw *drw = kbd->drw = drw_create(dpy, screen, root, kbd->w, kbd->h);

  // Setup fonts
  if (!drw_fontset_create(drw, &font, 1)) die("no fonts could be loaded");

  // Init color schemes
  for (int i = 0; i < SchemeLast; i++)
    kbd->scheme[i] = drw_scm_create(drw, colors[i], 2);

  drw_setscheme(drw, kbd->scheme[SchemeNorm]);

  // Create window
  Clr *clr = kbd->scheme[SchemeNorm];
  kbd->win = create_window(dpy, root, "bbkbd", kbd->w, kbd->h, kbd->x, kbd->y,
                           clr[ColFg].pixel, clr[ColBg].pixel);

  // Init keyboard
  keyboard_layout(kbd);

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

  if (kbd->keys) {
    for (int r = 0; r < kbd->rows; r++)
      free(kbd->keys[r]);
    free(kbd->keys);
  }
  free(kbd);
}
