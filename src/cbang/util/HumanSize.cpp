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

#include "HumanSize.h"

#include <cbang/String.h>

using namespace std;
using namespace cb;

#define KIBI 1024
#define MEBI (1024 * KIBI)
#define GIBI (1024 * MEBI)
#define TEBI (1024 * (uint64_t)GIBI)
#define PEBI (1024 * TEBI)
#define EXBI (1024 * PEBI)


string HumanSize::toString() const {
  if (size < KIBI) return String(size);
  else if (size < MEBI) return String::printf("%.02fKi", (double)size / KIBI);
  else if (size < GIBI) return String::printf("%.02fMi", (double)size / MEBI);
  else if (size < TEBI) return String::printf("%.02fGi", (double)size / GIBI);
  else if (size < PEBI) return String::printf("%.02fTi", (double)size / TEBI);
  else if (size < EXBI) return String::printf("%.02fPi", (double)size / PEBI);
  else return String::printf("%.02fEi", (double)size / EXBI);
}
