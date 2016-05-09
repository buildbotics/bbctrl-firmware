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

#ifndef CB_CONSTRAINT_H
#define CB_CONSTRAINT_H

#include <cbang/Exception.h>

#include <vector>
#include <string>


namespace cb {
  class Constraint {
  public:
    virtual ~Constraint() {}

    virtual void validate(bool value) const {}
    virtual void validate(const std::string &value) const {}
    virtual void validate(int64_t value) const {}
    virtual void validate(double value) const {}

    template <typename T>
    void validate(const std::vector<T> &value) const {
      for (unsigned i = 0; i < value.size(); i++)
        try {
          validate(value[i]);
        } catch (const Exception &e) {
          CBANG_THROWCS("Item " << i << " is invalid", e);
        }
    }

    virtual std::string getHelp() const = 0;
  };
}

#endif // CB_CONSTRAINT_H
