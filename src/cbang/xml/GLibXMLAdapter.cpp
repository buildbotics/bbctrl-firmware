/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#ifdef HAVE_GLIB
#include <cbang/xml/GLibXMLAdapter.h>

#include <cbang/String.h>
#include <cbang/Exception.h>

#include <fstream>

#include <glib.h>

using namespace std;
using namespace cb;

#define BUFFER_SIZE 4096


GError GLibXMLAdapter::gerror = {
  G_MARKUP_ERROR,
  G_MARKUP_ERROR_INVALID_CONTENT,
  (gchar *)"XML Reader error"
};


void GLibXMLAdapter::read(istream &stream) {
  typedef void (*start_element_func_t)(GMarkupParseContext *, const gchar *,
                                       const gchar **, const gchar **,
                                       gpointer data, GError **error);
  typedef void (*end_element_func_t)(GMarkupParseContext *, const gchar *,
                                     gpointer, GError **);
  typedef void (*text_func_t)(GMarkupParseContext *, const gchar *,
                              gsize, gpointer, GError **);

  GMarkupParser parser = {
    (start_element_func_t)&GLibXMLAdapter::startElement,
    (end_element_func_t)&GLibXMLAdapter::endElement,
    (text_func_t)&GLibXMLAdapter::text,
    0, // passthrough
    0, // error
  };

  GMarkupParseContext *context =
    g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, (void *)this, 0);
  if (!context) THROW("Failed to create XML parser.");

  try {
    while (!stream.eof() && !stream.fail()) {
      char buf[BUFFER_SIZE];

      stream.read(buf, BUFFER_SIZE);
      streamsize count = stream.gcount();

      GError *gerror = 0;
      if (count) {
        g_markup_parse_context_parse(context, buf, count, &gerror);

        if (error.get()) {
          Exception e(*error);
          error = 0;
          throw;
        }

        if (gerror)
          THROWS("Parse failed " << g_quark_to_string(gerror->domain) << ": "
                 << gerror->code << ": " << gerror->message);
      }
    }

  } catch (const Exception &e) {
    int line;
    int column;
    g_markup_parse_context_get_position(context, &line, &column);

    g_free(context);


    throw Exception(e.getMessage(),
                    FileLocation(getFilename(), line - 1, column), e);
  }
}

void GLibXMLAdapter::startElement(GMarkupParseContext *, const char *name,
                                  const char **attNames,
                                  const char **attValues, void *data,
                                  GError **error) {
  GLibXMLAdapter *reader = (GLibXMLAdapter *)data;

  try {
    XMLAttributes attrs;
    for (unsigned i = 0; attNames[i]; i++)
      attrs[attNames[i]] = attValues[i];

    reader->getHandler().startElement(name, attrs);

  } catch (const Exception &e) {
    reader->error = new Exception(e);
    *error = &gerror;
  }
}

void GLibXMLAdapter::endElement(GMarkupParseContext *, const char *name,
                                void *data, GError **error) {
  GLibXMLAdapter *reader = (GLibXMLAdapter *)data;

  try {
    reader->getHandler().endElement(name);

  } catch (const Exception &e) {
    reader->error = new Exception(e);
    *error = &gerror;
  }
}

void GLibXMLAdapter::text(GMarkupParseContext *, const char *text,
                          unsigned length, void *data, GError **error) {
  GLibXMLAdapter *reader = (GLibXMLAdapter *)data;

  try {
    reader->getHandler().text(string(text, length));

  } catch (const Exception &e) {
    reader->error = new Exception(e);
    *error = &gerror;
  }
}

#endif // HAVE_GLIB
