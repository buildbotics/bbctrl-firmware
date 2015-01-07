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

#ifndef CB_PARSER_H
#define CB_PARSER_H

#include <cbang/String.h>
#include <cbang/FileLocation.h>

#include <iostream>

namespace cb {
  class Parser : public FileLocation {
    std::istream &stream;
    std::string delims;
    bool caseSensitive;

    std::string current;

  public:
    Parser(std::istream &stream, const std::string &delims =
           cb::String::DEFAULT_DELIMS, bool caseSensitive = true) :
      stream(stream), delims(delims), caseSensitive(caseSensitive) {}

    const std::string &getDelims() const {return delims;}
    void setDelims(const std::string &delims) {this->delims = delims;}

    bool isCaseSensitive() const {return caseSensitive;}
    void setCaseSensitive(bool caseSensitive)
    {this->caseSensitive = caseSensitive;}

    std::string get();
    std::string advance();
    bool check(const std::string &token);
    void match(const std::string &token);

    std::string getLine(const std::string &lineDelims =
                        cb::String::DEFAULT_LINE_DELIMS);

  protected:
    char next();
  };
}

#endif // CB_PARSER_H

