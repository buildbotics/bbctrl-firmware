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

#include "Function.h"

#include "Arguments.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

using namespace std;
using namespace cb::Script;


ostream &Function::printHelpLine(ostream &stream) const {
  stream << "  " << getName();
  unsigned width = getName().length() + 2;

  if (!argHelp.empty()) {
    stream << ' ' << argHelp;
    width += 1 + argHelp.length();
  }

  stream << ' ';
  width++;

  return String::fill(stream, getHelp(), width, 30) << '\n';
}


void Function::validate(const Arguments &args) const {
  if (!args.size()) THROW("Internal error: Function eval missing name");

  unsigned size = args.size() - 1; // First arg is the function name
  if (size < minArgs || maxArgs < size) args.invalidNum();
}
