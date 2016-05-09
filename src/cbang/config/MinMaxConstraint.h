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

#ifndef CB_MIN_MAX_CONSTRAINT_H
#define CB_MIN_MAX_CONSTRAINT_H

#include "MinConstraint.h"
#include "MaxConstraint.h"

namespace cb {
  template <typename T>
  class MinMaxConstraint : public Constraint {
    MinConstraint<T> minimum;
    MaxConstraint<T> maximum;

  public:
    MinMaxConstraint(T minimum, T maximum) :
      minimum(minimum), maximum(maximum) {}

    // From Constraint
    void validate(int64_t value) const {
      minimum.validate(value);
      maximum.validate(value);
    }

    void validate(double value) const {
      minimum.validate(value);
      maximum.validate(value);
    }


    std::string getHelp() const {
      return minimum.getHelp() + " & " + maximum.getHelp();
    }
  };
}

#endif // CB_MIN_MAX_CONSTRAINT_H
