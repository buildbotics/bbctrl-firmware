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

#ifndef CBANG_XML_WRITER_H
#define CBANG_XML_WRITER_H

#include "XMLHandler.h"

#include <iostream>

namespace cb {
  class XMLWriter : public XMLHandler {
    std::ostream &stream;
    bool pretty;

    bool closed;
    bool startOfLine;
    unsigned depth;

  public:
    XMLWriter(std::ostream &stream, bool pretty = false) :
      stream(stream), pretty(pretty), closed(true), startOfLine(true),
      depth(0) {}

    void setPretty(bool x) {pretty = x;}
    bool getPretty() const {return pretty;}

    void entityRef(const std::string &name);

    // From XMLHandler
    void startElement(const std::string &name, const std::string &attrs);
    void startElement(const std::string &name,
                      const XMLAttributes &attrs = XMLAttributes());
    void endElement(const std::string &name);
    void text(const std::string &text);
    void comment(const std::string &text);

    void indent();
    void wrap();
    void simpleElement(const std::string &name,
                       const std::string &content = std::string(),
                       const XMLAttributes &attrs = XMLAttributes());

    static const std::string escape(const std::string &name);
  };
}

#endif // CBANG_XML_WRITER_H
