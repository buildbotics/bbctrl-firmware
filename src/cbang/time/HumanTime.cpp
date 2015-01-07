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

#include "HumanTime.h"

#include <cbang/String.h>

#include <iomanip>

using namespace std;
using namespace cb;


void HumanTime::print(ostream &stream) const {
  uint64_t t = time;
  uint64_t sec = t % 60; t /= 60;
  uint64_t min = t % 60; t /= 60;
  uint64_t hour = t % 24; t /= 24;
  uint64_t day = t % 365; t /= 365;

  if (t)    stream <<               t << "y ";
  if (day)  stream << setw(3) << day  << "d ";
  if (hour) stream << setw(2) << hour << "h ";
  if (min)  stream << setw(2) << min  << "m ";
  stream           << setw(2) << sec  << "s";
}
