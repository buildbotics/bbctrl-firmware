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

#ifndef CBANG_OPTION_CATEGORY_H
#define CBANG_OPTION_CATEGORY_H

#include "Option.h"

#include <cbang/SmartPointer.h>

#include <map>
#include <string>

namespace cb {
  class XMLHandler;

  namespace JSON {class Sink;}

  class OptionCategory {
    typedef std::map<const std::string, SmartPointer<Option> > options_t;
    options_t options;

    const std::string name;
    std::string description;
    bool hidden;

  public:
    OptionCategory(const std::string &name,
                   const std::string &description = std::string(),
                   bool hidden = false) :
      name(name), description(description), hidden(hidden) {}

    const std::string &getName() const {return name;}

    bool isEmpty() const {return options.empty();}
    bool hasSetOption() const;

    void setDescription(const std::string &x) {description = x;}
    const std::string &getDescription() const {return description;}

    void setHidden(bool x) {hidden = x;}
    bool getHidden() const {return hidden;}

    void add(const SmartPointer<Option> &option);

    void write(JSON::Sink &sink, bool config = false,
               const std::string &delims = Option::DEFAULT_DELIMS) const;
    void write(XMLHandler &handler, uint32_t flags) const;
    void printHelpTOC(XMLHandler &handler, const std::string &prefix) const;
    void printHelp(XMLHandler &handler, const std::string &prefix) const;
    void printHelp(std::ostream &stream) const;
  };
}

#endif // CBANG_OPTION_CATEGORY_H

