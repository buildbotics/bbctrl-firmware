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

#ifdef HAVE_OPENSSL
#include <openssl/rand.h>
#else
#include <stdlib.h>
#endif

using namespace cb;


Random::Random(Inaccessible) {
  // On systems with /dev/urandom the generator is automatically seeded

#if defined(_WIN32) && defined(HAVE_OPENSSL)
  RAND_screen();
#endif
}


void Random::addEntropy(const void *buffer, uint32_t bytes, double entropy) {
#ifdef HAVE_OPENSSL
  RAND_add(buffer, bytes, entropy ? entropy : bytes);

#else
  if (sizeof(unsigned) <= bytes) srand(*(unsigned *)buffer);
#endif
}


void Random::bytes(void *buffer, uint32_t bytes) {
#ifdef HAVE_OPENSSL
  RAND_pseudo_bytes((unsigned char *)buffer, bytes);

#else
  while (1 < bytes) {
    bytes -= 2;
    ((uint16_t *)buffer)[bytes / 2] = ::rand();
  }

  if (bytes) *(uint8_t *)buffer = ::rand();
#endif
}
