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
#include "button.h"
#include "drw.h"
#include "util.h"
#include "config.h"

#include <signal.h>
#include <locale.h>

#define DEFAULT_FONT "DejaVu Sans:size=18"

static const char *font = DEFAULT_FONT;
static float button_x = 1;
static float button_y = 0;
static bool running = true;
static const char *show_cmd = 0;
static const char *hide_cmd = 0;
static int space = 4;


void signaled(int sig) {
  running = false;
  print_dbg("Signal %d received\n", sig);
}


void usage(char *argv0, int ret) {
  const char *usage =
    "usage: %s [-hdb] [-f <font>] [-b <x> <y>]\n"
    "Options:\n"
    "  -h         - Print this help screen and exit\n"
    "  -d         - Enable debug\n"
    "  -f <font>  - Font string, default: " DEFAULT_FONT "\n"
    "  -b <x> <y> - Button screen position. Values between 0 and 1.\n"
    "  -S <cmd>   - Command to run before showing the keyboard.\n"
    "  -H <cmd>   - Command to run after hiding the keyboard.\n"
    "  -s <int>   - Space between buttons.\n";

  fprintf(ret ? stderr : stdout, usage, argv0);
  exit(ret);
}


void parse_args(int argc, char *argv[]) {
  for (int i = 1; argv[i]; i++) {
    if (!strcmp(argv[i], "-h")) usage(argv[0], 0);
    else if (!strcmp(argv[i], "-d")) debug = true;
    else if (!strcmp(argv[i], "-f")) {
      if (argc - 1 <= i) usage(argv[0], 1);
      font = argv[++i];

    } else if (!strcmp(argv[i], "-b")) {
      if (argc - 2 <= i) usage(argv[0], 1);
      button_x = atof(argv[++i]);
      button_y = atof(argv[++i]);

    } else if (!strcmp(argv[i], "-S")) {
      if (argc - 1 <= i) usage(argv[0], 1);
      show_cmd = argv[++i];

    } else if (!strcmp(argv[i], "-H")) {
      if (argc - 1 <= i) usage(argv[0], 1);
      hide_cmd = argv[++i];

    } else if (!strcmp(argv[i], "-s")) {
      if (argc - 1 <= i) usage(argv[0], 1);
      space = atoi(argv[++i]);

    } else {
      fprintf(stderr, "Invalid argument: %s\n", argv[i]);
      usage(argv[0], 1);
    }
  }
}


void button_callback(Keyboard *kbd) {
  if (!kbd->visible && show_cmd) system(show_cmd);
  keyboard_toggle(kbd);
  if (!kbd->visible && hide_cmd) system(hide_cmd);
}


int main(int argc, char *argv[]) {
  signal(SIGTERM, signaled);
  signal(SIGINT, signaled);

  parse_args(argc, argv);

  // Check locale support
  if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
    fprintf(stderr, "warning: no locale support");

  // Init
  Display *dpy = XOpenDisplay(0);
  if (!dpy) die("cannot open display");

  Button *btn = button_create(dpy, button_x, button_y, 55, 35, font);
  Keyboard *kbd = keyboard_create(dpy, keys, space, font, colors);

  button_set_callback(btn, button_callback, kbd);

  // Event loop
  while (running) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms

    int xfd = ConnectionNumber(dpy);
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(xfd, &fds);
    int r = select(xfd + 1, &fds, 0, 0, &tv);

    if (r == -1) break;

    while (r && XPending(dpy)) {
      XEvent ev;
      XNextEvent(dpy, &ev);

      if (ev.xany.window == kbd->win)
        keyboard_event(kbd, &ev);

      if (ev.xany.window == btn->win)
        button_event(btn, &ev);
    }
  }

  // Cleanup
  button_destroy(btn);
  keyboard_destroy(kbd);
  XCloseDisplay(dpy);

  return 0;
}
