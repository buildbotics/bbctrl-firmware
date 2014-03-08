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

#ifdef HAVE_EXPAT
#include "ExpatXMLAdapter.h"

#include <expat.h>

#include <cbang/String.h>

using namespace std;
using namespace cb;

static const int BUFFER_SIZE = 4096;


ExpatXMLAdapter::ExpatXMLAdapter() : parser(XML_ParserCreate("UTF-8")) {
  XML_SetElementHandler
    ((XML_Parser)parser, (XML_StartElementHandler)&ExpatXMLAdapter::start,
     (XML_EndElementHandler)&ExpatXMLAdapter::end);
  XML_SetCharacterDataHandler
    ((XML_Parser)parser, (XML_CharacterDataHandler)&ExpatXMLAdapter::text);
  XML_SetUserData((XML_Parser)parser, this);
}


ExpatXMLAdapter::~ExpatXMLAdapter() {
  XML_ParserFree((XML_Parser)parser);
  parser = 0;
}


void ExpatXMLAdapter::read(istream &stream) {
  streamsize count;

  do {
    void *buf = XML_GetBuffer((XML_Parser)parser, BUFFER_SIZE);
    stream.read((char *)buf, BUFFER_SIZE);
    count = stream.gcount();

    if (!XML_ParseBuffer((XML_Parser)parser, count, count == 0) && !error) {
      XML_Error code = XML_GetErrorCode((XML_Parser)parser);
      int line = XML_GetCurrentLineNumber((XML_Parser)parser);
      int column = XML_GetCurrentColumnNumber((XML_Parser)parser);

      error = new Exception(string("Parse failed: ") + String(code) +
                            ": " + XML_ErrorString(code),
                            FileLocation(getFilename(), line, column));
    }

    if (error.get()) break;
  } while (count);

  names.clear();

  if (error.get()) {
    Exception e(*error.get());
    error = 0;

    throw e;
  }
}


void ExpatXMLAdapter::setError(const Exception &e) {
  if (error.get()) return;

  int line = XML_GetCurrentLineNumber((XML_Parser)parser);
  int column = XML_GetCurrentColumnNumber((XML_Parser)parser);

  error = new Exception(e.getMessage(),
                        FileLocation(getFilename(), line, column), e);
}


void ExpatXMLAdapter::start(ExpatXMLAdapter *adapter, const char *name,
                            const char **_attrs) {
  if (adapter->hasError()) return;
  try {
    adapter->names.push_back(name);

    XMLAttributes attrs;
    for (unsigned i = 0; _attrs[i]; i += 2)
      attrs[_attrs[i]] = _attrs[i + 1];

    adapter->getHandler().startElement(name, attrs);
  } catch (const Exception &e) {
    adapter->setError(e);
  }
}


void ExpatXMLAdapter::end(ExpatXMLAdapter *adapter, void *data,
                          const char *_name) {
  if (adapter->hasError()) return;
  try {
    string name = adapter->names.back();
    adapter->names.pop_back();

    adapter->getHandler().endElement(name);
  } catch (const Exception &e) {
    adapter->setError(e);
  }
}


void ExpatXMLAdapter::text(ExpatXMLAdapter *adapter,
                           const char *text, int len) {
  if (adapter->hasError()) return;
  try {
    adapter->getHandler().text(string(text, len));
  } catch (const Exception &e) {
    adapter->setError(e);
  }
}

#endif // HAVE_EXPAT
