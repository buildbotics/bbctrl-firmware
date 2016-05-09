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

#ifndef CBANG_BUFFER_H
#define CBANG_BUFFER_H

#include <cbang/Exception.h>

#include <istream>
#include <ostream>

namespace cb {
  class Buffer {
  public:
    virtual ~Buffer() {}

    virtual bool isFull() const {return !getSpace();}
    virtual bool isEmpty() const {return !getFill();}

    virtual unsigned getFill() const {return 0;}
    virtual unsigned getSpace() const {return 0;}

    virtual unsigned read(char *dst, unsigned length)
    {CBANG_THROW("Not implemented");}
    virtual unsigned write(const char *src, unsigned length)
    {CBANG_THROW("Not implemented");}

    virtual unsigned writeTo(std::ostream &stream);
    virtual unsigned readFrom(std::istream &stream);
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, Buffer &b) {
    b.writeTo(stream);
    return stream;
  }


  static inline
  std::istream &operator>>(std::istream &stream, Buffer &b) {
    b.readFrom(stream);
    return stream;
  }
}

#endif // CBANG_BUFFER_H
