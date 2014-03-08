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

#ifndef CBANG_TOKENIZER_H
#define CBANG_TOKENIZER_H

#include "Token.h"
#include "Scanner.h"

#include <cbang/Exception.h>
#include <cbang/String.h>


namespace cb {
  template <class ENUM_T>
  class Tokenizer : public ENUM_T {
  protected:
    Scanner &scanner;

    typedef Token<ENUM_T> Token_T;
    Token_T current;

  public:
    Tokenizer(cb::Scanner &scanner) : scanner(scanner) {}

    const Scanner &getScanner() const {return scanner;}
    Scanner &getScanner() {return scanner;}
    const LocationRange &getLocation() const {return peek().getLocation();}

    bool isType(ENUM_T type) const {return peek().getType() == type;}
    ENUM_T getType() const {return peek().getType();}
    const std::string &getValue() const {return peek().getValue();}

    bool hasMore() {
      if (current.getType() == Token_T::eof() && scanner.hasMore()) advance();
      return current.getType() != Token_T::eof();
    }

    const Token_T &peek() const {return current;}

    Token_T advance() {
      Token_T last = current;
      next();
      return last;
    }

    Token_T match(ENUM_T type) {
      if (!hasMore()) THROWS("Expected " << type << " found end of stream");
      if (type != getType())
        THROWS("Expected " << type << " found " << getType());

      return advance();
    }

    bool consume(ENUM_T type) {
      if (!hasMore() || peek().getType() != type) return false;
      advance();
      return true;
    }

    virtual void next() = 0;
  };
}

#endif // CBANG_TOKENIZER_H

