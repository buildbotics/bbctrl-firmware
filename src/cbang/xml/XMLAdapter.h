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

#ifndef CBANG_XML_ADAPTER_H
#define CBANG_XML_ADAPTER_H

#include "XMLHandler.h"

#include <string>
#include <vector>

namespace cb {
  class XMLAdapter {
    std::vector<XMLHandler *> handlers;
    std::string filename;

  public:
    virtual ~XMLAdapter() {}

    void pushHandler(XMLHandler *handler);
    void popHandler();
    XMLHandler &getHandler();

    void setFilename(const std::string &x) {filename = x;}
    const std::string &getFilename() {return filename;}

    void read(const std::string &filename);
    virtual void read(std::istream &stream) = 0;

    static XMLAdapter *create();
  };
}

#endif // CBANG_XML_ADAPTER_H

