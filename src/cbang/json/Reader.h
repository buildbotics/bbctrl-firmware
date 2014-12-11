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

#ifndef CBANG_JSON_READER_H
#define CBANG_JSON_READER_H

#include <cbang/io/InputSource.h>
#include <cbang/SmartPointer.h>


namespace cb {
  namespace JSON {
    class Value;
    class List;
    class Dict;
    class Sync;

    class Reader {
      unsigned line;
      unsigned column;
      InputSource src;
      std::istream &stream;

    public:
      Reader(const InputSource &src) :
      line(0), column(0), src(src), stream(src.getStream()) {}

      void parse(Sync &sync);
      SmartPointer<Value> parse();
      static SmartPointer<Value> parse(const InputSource &src);

      unsigned getLine() const {return line;}
      unsigned getColumn() const {return column;}

      char get();
      char next();
      void advance() {stream.get();}
      bool tryMatch(char c);
      char match(const char *chars);
      bool good() const {return stream.good();}

      const std::string parseKeyword();
      void parseNull();
      bool parseBoolean();
      double parseNumber();
      const std::string parseString();
      void parseList(Sync &sync);
      void parseDict(Sync &sync);

      void error(const std::string &msg) const;

      static std::string unescape(const std::string &s);
    };
  }
}

#endif // CBANG_JSON_READER_H

