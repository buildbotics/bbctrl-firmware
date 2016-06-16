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

#ifndef CB_JS_ARGUMENTS_H
#define CB_JS_ARGUMENTS_H

#include "Value.h"
#include "V8.h"


namespace cb {
  namespace js {
    class Signature;

    class Arguments {
      const v8::Arguments &args;
      const Signature &sig;

      unsigned positional;
      Value keyWord;

    public:
      Arguments(const v8::Arguments &args, const Signature &sig);

      Value getThis() const {return Value(args.This());}
      unsigned getCount() const;

      const Signature &getSignature() const {return sig;}
      std::string getName(unsigned index) const;

      bool has(unsigned index) const;
      bool has(const std::string &name) const;

      Value get(unsigned index) const;
      Value get(const std::string &name) const;

      bool getBoolean(const std::string &name) const;
      double getNumber(const std::string &name) const;
      int32_t getInt32(const std::string &name) const;
      uint32_t getUint32(const std::string &name) const;
      std::string getString(const std::string &name) const;

      bool getBoolean(unsigned index) const;
      double getNumber(unsigned index) const;
      int32_t getInt32(unsigned index) const;
      uint32_t getUint32(unsigned index) const;
      std::string getString(unsigned index) const;

      std::string toString() const;
      std::ostream &write(std::ostream &stream,
                          const std::string &separator = " ") const;

      Value operator[](const std::string &name) const {return get(name);}
      Value operator[](unsigned i) const {return get(i);}
    };


    static inline
    std::ostream &operator<<(std::ostream &stream, const Arguments &args) {
      return args.write(stream);
    }
  }
}

#endif // CB_JS_ARGUMENTS_H
