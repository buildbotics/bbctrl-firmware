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

#include "drw.h"
#include "util.h"

#include <string.h>
#include <stdint.h>


#define UTF_INVALID 0xFFFD
#define UTF_SIZ     4

static const uint8_t utfbyte[] = {0x80,    0, 0xC0, 0xE0, 0xF0};
static const uint8_t utfmask[] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[] = {       0,    0,  0x80,  0x800, 0x10000};
static const long utfmax[] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};


static long utf8decodebyte(const char c, size_t *i) {
  for (*i = 0; *i < (UTF_SIZ + 1); ++(*i))
    if (((uint8_t)c & utfmask[*i]) == utfbyte[*i])
      return (uint8_t)c & ~utfmask[*i];

  return 0;
}


static size_t utf8validate(long *u, size_t i) {
  if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
    *u = UTF_INVALID;

  for (i = 1; *u > utfmax[i]; ++i) continue;

  return i;
}


static size_t utf8decode(const char *c, long *u, size_t clen) {
  size_t i, j, len, type;

  *u = UTF_INVALID;
  if (!clen) return 0;
  long udecoded = utf8decodebyte(c[0], &len);

  if (!BETWEEN(len, 1, UTF_SIZ))
    return 1;

  for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
    udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
    if (type) return j;
  }

  if (j < len) return 0;
  *u = udecoded;
  utf8validate(u, len);

  return len;
}


Drw *drw_create(Display *dpy, int screen, Window root, unsigned w, unsigned h) {
  Drw *drw = calloc(1, sizeof(Drw));

  drw->dpy = dpy;
  drw->screen = screen;
  drw->root = root;
  drw->w = w;
  drw->h = h;
  drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
  drw->gc = XCreateGC(dpy, root, 0, 0);
  XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

  return drw;
}


void drw_resize(Drw *drw, unsigned w, unsigned h) {
  if (!drw) return;

  drw->w = w;
  drw->h = h;
  if (drw->drawable) XFreePixmap(drw->dpy, drw->drawable);
  drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
}


void drw_free(Drw *drw) {
  XFreePixmap(drw->dpy, drw->drawable);
  XFreeGC(drw->dpy, drw->gc);
  drw_fontset_free(drw->fonts);
  free(drw);
}


/// This function is an implementation detail. Library users should use
/// drw_fontset_create instead.
static Fnt *xfont_create(Drw *drw, const char *fontname,
                         FcPattern *fontpattern) {
  XftFont *xfont = 0;
  FcPattern *pattern = 0;

  if (fontname) {
    // Using the pattern found at font->xfont->pattern does not yield the
    // same substitution results as using the pattern returned by
    // FcNameParse; using the latter results in the desired fallback
    // behaviour whereas the former just results in missing-character
    // rectangles being drawn, at least with some fonts.
    if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
      fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
      return 0;
    }

    if (!(pattern = FcNameParse((FcChar8 *) fontname))) {
      fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n",
              fontname);
      XftFontClose(drw->dpy, xfont);
      return 0;
    }

  } else if (fontpattern) {
    if (!(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {
      fprintf(stderr, "error, cannot load font from pattern.\n");
      return 0;
    }

  } else die("no font specified.");

  // Do not allow using color fonts. This is a workaround for a BadLength
  // error from Xft with color glyphs. Modelled on the Xterm workaround. See
  // https://bugzilla.redhat.com/show_bug.cgi?id=1498269
  // https://lists.suckless.org/dev/1701/30932.html
  // https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=916349
  // and lots more all over the internet.
  FcBool iscol;
  if (FcPatternGetBool(xfont->pattern, FC_COLOR, 0, &iscol) == FcResultMatch &&
      iscol) {
    XftFontClose(drw->dpy, xfont);
    return 0;
  }

  Fnt *font = calloc(1, sizeof(Fnt));
  font->xfont = xfont;
  font->pattern = pattern;
  font->h = xfont->ascent + xfont->descent;
  font->dpy = drw->dpy;

  return font;
}


static void xfont_free(Fnt *font) {
  if (!font) return;
  if (font->pattern) FcPatternDestroy(font->pattern);
  XftFontClose(font->dpy, font->xfont);
  free(font);
}


Fnt *drw_fontset_create(Drw *drw, const char *fonts[], size_t count) {
  Fnt *cur, *ret = 0;

  if (!drw || !fonts) return 0;

  for (size_t i = 1; i <= count; i++)
    if ((cur = xfont_create(drw, fonts[count - i], 0))) {
      cur->next = ret;
      ret = cur;
    }

  return drw->fonts = ret;
}


void drw_fontset_free(Fnt *font) {
  if (font) {
    drw_fontset_free(font->next);
    xfont_free(font);
  }
}


void drw_clr_create(Drw *drw, Clr *dest, const char *clrname) {
  if (!drw || !dest || !clrname) return;

  if (!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen),
                         DefaultColormap(drw->dpy, drw->screen),
                         clrname, dest))
    die("error, cannot allocate color '%s'", clrname);
}


/// Wrapper to create color schemes. The caller has to call free(3) on the
/// returned color scheme when done using it.
Clr *drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount) {
  Clr *ret;

  // need at least two colors for a scheme
  if (!drw || !clrnames || clrcount < 2 ||
      !(ret = calloc(clrcount, sizeof(XftColor))))
    die("error, cannot create color scheme (drw=%d) (clrcount=%d)", drw,
        clrcount);

  for (size_t i = 0; i < clrcount; i++)
    drw_clr_create(drw, &ret[i], clrnames[i]);

  return ret;
}


void drw_setfontset(Drw *drw, Fnt *set) {if (drw) drw->fonts = set;}
void drw_setscheme(Drw *drw, Clr *scm) {if (drw) drw->scheme = scm;}


void drw_rect(Drw *drw, int x, int y, unsigned w, unsigned h, int filled,
              int invert) {
  if (!drw || !drw->scheme) return;

  XSetForeground(drw->dpy, drw->gc,
                 invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);

  if (filled) XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
  else XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}


int drw_text(Drw *drw, int x, int y, unsigned w, unsigned h, unsigned lpad,
             const char *text, int invert) {
  char buf[1024];
  int ty;
  XftDraw *d = 0;
  Fnt *curfont, *nextfont;
  size_t i, len;
  int utf8strlen, utf8charlen, render = x || y || w || h;
  long utf8codepoint = 0;
  const char *utf8str;
  FcCharSet *fccharset;
  FcPattern *fcpattern;
  FcPattern *match;
  XftResult result;
  int charexists = 0;

  if (!drw || (render && !drw->scheme) || !text || !drw->fonts) return 0;

  if (!render) w = ~w;
  else {
    XSetForeground(drw->dpy, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);
    XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
    d = XftDrawCreate(drw->dpy, drw->drawable,
                      DefaultVisual(drw->dpy, drw->screen),
                      DefaultColormap(drw->dpy, drw->screen));
    x += lpad;
    w -= lpad;
  }

  Fnt *usedfont = drw->fonts;

  while (1) {
    utf8strlen = 0;
    utf8str = text;
    nextfont = 0;

    while (*text) {
      utf8charlen = utf8decode(text, &utf8codepoint, UTF_SIZ);
      for (curfont = drw->fonts; curfont; curfont = curfont->next) {
        charexists =
          charexists || XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);
        if (charexists) {
          if (curfont == usedfont) {
            utf8strlen += utf8charlen;
            text += utf8charlen;

          } else nextfont = curfont;
          break;
        }
      }

      if (!charexists || nextfont) break;
      else charexists = 0;
    }

    if (utf8strlen) {
      unsigned ew = 0;
      drw_font_getexts(usedfont, utf8str, utf8strlen, &ew, 0);

      // shorten text if necessary
      for (len = MIN(utf8strlen, sizeof(buf) - 1); len && ew > w; len--)
        drw_font_getexts(usedfont, utf8str, len, &ew, 0);

      if (len) {
        memcpy(buf, utf8str, len);
        buf[len] = '\0';
        if (len < utf8strlen)
          for (i = len; i && i > len - 3; buf[--i] = '.') continue;

        if (render) {
          ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;
          XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],
                            usedfont->xfont, x, ty, (XftChar8 *)buf, len);
        }

        x += ew;
        w -= ew;
      }
    }

    if (!*text) break;
    else if (nextfont) {
      charexists = 0;
      usedfont = nextfont;

    } else {
      // Regardless of whether or not a fallback font is found, the
      // character must be drawn.
      charexists = 1;

      fccharset = FcCharSetCreate();
      FcCharSetAddChar(fccharset, utf8codepoint);

      if (!drw->fonts->pattern)
        // Refer to the comment in xfont_create for more information.
        die("the first font in the cache must be loaded from a font string.");

      fcpattern = FcPatternDuplicate(drw->fonts->pattern);
      FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
      FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);
      FcPatternAddBool(fcpattern, FC_COLOR, FcFalse);

      FcConfigSubstitute(0, fcpattern, FcMatchPattern);
      FcDefaultSubstitute(fcpattern);
      match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

      FcCharSetDestroy(fccharset);
      FcPatternDestroy(fcpattern);

      if (match) {
        usedfont = xfont_create(drw, 0, match);

        if (usedfont &&
            XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {
          for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
            continue;
          curfont->next = usedfont;

        } else {
          xfont_free(usedfont);
          usedfont = drw->fonts;
        }
      }
    }
  }

  if (d) XftDrawDestroy(d);

  return x + (render ? w : 0);
}


void drw_map(Drw *drw, Window win, int x, int y, unsigned w, unsigned h) {
  if (!drw) return;
  XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
}


void drw_sync(Drw *drw) {XSync(drw->dpy, False);}


unsigned drw_fontset_getwidth(Drw *drw, const char *text) {
  if (!drw || !drw->fonts || !text) return 0;
  return drw_text(drw, 0, 0, 0, 0, 0, text, 0);
}


void drw_font_getexts(Fnt *font, const char *text, unsigned len, unsigned *w,
                      unsigned *h) {
  XGlyphInfo ext;

  if (!font || !text) return;

  XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
  if (w) *w = ext.xOff;
  if (h) *h = font->h;
}
