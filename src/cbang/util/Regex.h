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

#pragma once

#include <cbang/SmartPointer.h>

#include <vector>
#include <string>


namespace cb {
  class Regex {
    struct private_t;
    SmartPointer<private_t> pri;

  public:
    typedef enum {
      TYPE_POSIX,
      TYPE_PERL,
      TYPE_BOOST,
    } type_t;

  protected:
    type_t type;

  public:
    class Match : public std::vector<std::string> {
      struct private_t;
      SmartPointer<private_t> pri;
      type_t type;

    public:
      Match(type_t type = TYPE_POSIX);
      std::string format(const std::string &fmt) const;

      unsigned position(unsigned i = 0) const;

      friend class Regex;
    };


    Regex(const std::string &pattern, type_t type = TYPE_POSIX);

    bool match(const std::string &s) const;
    bool match(const std::string &s, Match &m) const;

    bool search(const std::string &s) const;
    bool search(const std::string &s, Match &m) const;

    std::string replace(const std::string &s, const std::string &r) const;
  };
}
