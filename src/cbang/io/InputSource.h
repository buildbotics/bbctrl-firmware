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

#ifndef CBANG_INPUT_SOURCE_H
#define CBANG_INPUT_SOURCE_H

#include <cbang/SmartPointer.h>
#include <cbang/util/Named.h>

#include <string>
#include <iostream>


namespace cb {
  class Buffer;
  struct Resource;

  class InputSource : public Named {
    cb::SmartPointer<std::istream> streamPtr;
    std::istream &stream;
    mutable std::streamsize length;

  public:
    InputSource(Buffer &buffer, const std::string &name = "<buffer>");
    InputSource(const char *array, std::streamsize length,
                const std::string &name = "<memory>");
    InputSource(const std::string &filename);
    InputSource(std::istream &stream, const std::string &name = std::string(),
                std::streamsize length = -1);
    InputSource(const Resource &resource);
    virtual ~InputSource() {} // Compiler needs this

    virtual std::istream &getStream() const {return stream;}
    virtual std::streamsize getLength() const;
  };
}

#endif // CBANG_INPUT_SOURCE_H

