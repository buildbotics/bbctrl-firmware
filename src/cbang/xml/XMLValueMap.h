/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#ifndef CBANG_XML_VALUE_MAP_H
#define CBANG_XML_VALUE_MAP_H

#include "XMLHandler.h"

#include <string>
#include <iostream>

namespace cb {
  class XMLWriter;

  class XMLValueMap : public XMLHandler {
    const std::string root;

    std::string filename;
    std::string xmlValue;
    bool xmlValueSet;
    unsigned depth;

  public:
    XMLValueMap(const std::string &root);

    void setFilename(const std::string &x) {filename = x;}

    void read(const std::string &filename);
    virtual void read(std::istream &stream);
    void write(const std::string &filename) const;
    virtual void write(std::ostream &stream) const;

    void writeXMLValue(XMLWriter &writer, const std::string &name,
                       const std::string &value) const;

    virtual void writeXMLValues(XMLWriter &writer) const = 0;
    virtual void setXMLValue(const std::string &name,
                             const std::string &value) = 0;

    // From XMLHandler
    void startElement(const std::string &name, const XMLAttributes &attrs);
    void endElement(const std::string &name);
    void text(const std::string &text);
  };

  inline std::istream &operator>>(std::istream &stream, XMLValueMap &o) {
    o.read(stream);
    return stream;
  }

  inline std::ostream &operator<<(std::ostream &stream, const XMLValueMap &o) {
    o.write(stream);
    return stream;
  }
}

#endif // CBANG_XML_VALUE_MAP_H

