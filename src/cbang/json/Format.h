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

#ifndef CB_JSON_FORMAT_H
#define CB_JSON_FORMAT_H

#include "Value.h"
#include "Writer.h"


namespace cb {
  namespace JSON {
    struct Format {
      Value &value;
      unsigned level;
      bool compact;
      Writer::output_mode_t mode;

      Format(Value &value, unsigned level = 0, bool compact = false,
             Writer::output_mode_t mode = Writer::JSON_MODE) :
        value(value), level(level), compact(compact), mode(mode) {}
    };


    static inline
    std::ostream &operator<<(std::ostream &stream, const Format &f) {
      Writer writer(stream, f.level, f.compact, f.mode);
      f.value.write(writer);
      return stream;
    }
  }
}

#endif // CB_JSON_FORMAT_H