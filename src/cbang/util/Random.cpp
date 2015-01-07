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

#include "Random.h"

#include <cbang/Exception.h>
#include <cbang/Zap.h>

#include <cbang/time/Time.h>

#include <openssl/rand.h>

using namespace cb;

SINGLETON_DECL(Random);


Random::Random(Inaccessible) {
  // On systems with /dev/urandom the generator is automatically seeded

#ifdef _WIN32
  RAND_screen();
#endif
}


void Random::addEntropy(const void *buffer, uint32_t bytes, double entropy) {
  RAND_add(buffer, bytes, entropy ? entropy : bytes);
}


void Random::bytes(void *buffer, uint32_t bytes) {
  RAND_pseudo_bytes((unsigned char *)buffer, bytes);
}
