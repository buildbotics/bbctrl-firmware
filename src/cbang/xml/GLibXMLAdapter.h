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
#ifndef CBANG_GLIB_XMLADAPTER_H
#define CBANG_GLIB_XMLADAPTER_H

#include <cbang/xml/XMLAdapter.h>
#include <cbang/SmartPointer.h>
#include <cbang/Exception.h>

typedef struct _GMarkupParseContext GMarkupParseContext;
typedef struct _GError GError;

namespace cb {
  /**
   * A C++ adapter class for the glib simple XML parser.
   */
  class GLibXMLAdapter : public XMLAdapter {
    static GError gerror;
    SmartPointer<Exception> error;

  public:
    virtual void read(std::istream &stream);

  private:
    static void startElement(GMarkupParseContext *context, const char *name,
                             const char **attNames, const char **attValues,
                             void *data, GError **error);
    static void endElement(GMarkupParseContext *context, const char *name,
                           void *data, GError **error);
    static void text(GMarkupParseContext *context, const char *text,
                     unsigned length, void *data, GError **error);
  };
}

#endif // CBANG_GLIB_XMLADAPTER_H
#endif // HAVE_GLIB
