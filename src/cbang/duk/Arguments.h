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

#ifndef CB_DUK_ARGUMENTS_H
#define CB_DUK_ARGUMENTS_H

#include "Signature.h"

#include <string>
#include <ostream>


namespace cb {
  namespace duk {
    class Context;

    class Arguments {
      Context &ctx;
      const Signature &sig;

      unsigned positional;
      int keyword;

    public:
      Arguments(Context &ctx, const Signature &sig);

      Context &getContext() const {return ctx;}
      const Signature &getSignature() const {return sig;}

      bool has(unsigned index) const;
      bool has(const std::string &key) const;

      bool toBoolean(const std::string &key);
      double toNumber(const std::string &key);
      int toInteger(const std::string &key);
      std::string toString(const std::string &key);

      bool toBoolean(unsigned index);
      double toNumber(unsigned index);
      int toInteger(unsigned index);
      std::string toString(unsigned index);

      std::string toString() const;
      std::ostream &write(std::ostream &stream,
                          const std::string &separator = " ") const;

    protected:
      void push(const std::string &key);
      void push(unsigned index);
    };


    static inline
    std::ostream &operator<<(std::ostream &stream, const Arguments &args) {
      return args.write(stream);
    }
  }
}

#endif // CB_DUK_ARGUMENTS_H
