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

#ifndef CBANG_SCRIPT_FUNCTION_H
#define CBANG_SCRIPT_FUNCTION_H

#include "Entity.h"

#include <string>

namespace cb {
  namespace Script {
    class Function : public Entity {
      std::string argHelp;
      unsigned minArgs;
      unsigned maxArgs;
      bool autoEvalArgs;

    public:
      Function(const std::string &name, unsigned minArgs = 0,
               unsigned maxArgs = 0, const std::string &help = "",
               const std::string &argHelp = "", bool autoEvalArgs = true) :
        Entity(name, help), argHelp(argHelp), minArgs(minArgs),
        maxArgs(maxArgs), autoEvalArgs(autoEvalArgs) {}

      const std::string &getArgHelp() const {return argHelp;}
      unsigned getMinArgs() const {return minArgs;}
      unsigned getMaxArgs() const {return maxArgs;}

      // From Entity
      std::ostream &printHelpLine(std::ostream &stream) const;
      entity_t getType() const {return FUNCTION;}
      void validate(const Arguments &args) const;
      bool evalArgs() const {return autoEvalArgs;}
    };
  }
}

#endif // CBANG_SCRIPT_FUNCTION_H
