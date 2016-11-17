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

#ifndef CB_CHAKRA_VALUE_H
#define CB_CHAKRA_VALUE_H

#include <cbang/Exception.h>

#include <string>


namespace cb {
  namespace chakra {
    class Value {
      void *ref;

    public:
      Value(void *ref) : ref(ref) {}
      Value(const std::string &value);

      std::string toString() const;
      int toInteger() const;

      bool has(const std::string &key) const;
      Value get(const std::string &key) const;

      operator void * () const {return ref;}

      static bool hasException();
      static Value getException();
    };
  }
}


#define CHAKRA_CHECK(CMD)                                           \
  do {                                                              \
    JsErrorCode err = CMD;                                          \
    if (err != JsNoError)                                           \
      THROWS(#CMD << " failed with 0x" << std::hex << err);         \
  } while (0)


#endif // CB_CHAKRA_VALUE_H
