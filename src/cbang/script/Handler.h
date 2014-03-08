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

#ifndef CBANG_SCRIPT_HANDLER_H
#define CBANG_SCRIPT_HANDLER_H


#include <ostream>
#include <string>

namespace cb {
  namespace Script {
    class Context;
    class Arguments;

    class Handler {
    public:
      virtual ~Handler() {}

      virtual bool eval(const Context &ctx) = 0;

      std::string eval(const std::string &s);
      void eval(std::ostream &stream, const std::string &s);

      static void eval(const Context &ctx, const char *s);
      static void eval(const Context &ctx, const std::string &s);
      static void eval(const Context &ctx, const char *s, unsigned length);
      static void evalf(const Context &ctx, const char *s, ...);
      static void parse(Arguments &args, const std::string &s);
      static void exec(const Context &ctx, const std::string &script);

      void exec(std::ostream &stream, const std::string &script);
    };
  }
}

// NOTE: This breaks a circular dependency
#include "Context.h"

#endif // CBANG_SCRIPT_HANDLER_H

