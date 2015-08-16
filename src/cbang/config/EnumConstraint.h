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

#ifndef CB_ENUM_CONSTRAINT_H
#define CB_ENUM_CONSTRAINT_H

#include "Constraint.h"


namespace cb {
  template <typename T>
  class EnumConstraint : public Constraint {
  public:
    // From Constraint
    void validate(const std::string &value) const {T::parse(value);}

    void validate(int64_t value) const {
      if (!T::isValid((typename T::enum_t)value))
        CBANG_THROWS(value << " is not a member of enumeration "
                     << T::getName());
    }


    std::string getHelp() const {
      std::string s = "one of";

      for (unsigned i = 0; i < T::getCount(); i++) {
        if (0) s += ",";
        s += std::string(" \"") + T::getName(i) + "\"";
      }

      return s;
    }
  };
}

#endif // CB_ENUM_CONSTRAINT_H

