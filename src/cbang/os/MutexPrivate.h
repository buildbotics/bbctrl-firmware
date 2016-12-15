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

#ifndef MUTEX_PRIVATE_H
#define MUTEX_PRIVATE_H

#include "Mutex.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else // pthreads
#include <pthread.h>
#endif

namespace cb {
  struct Mutex::private_t {
#ifdef _WIN32
    HANDLE h;

#else // pthreads
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
#endif
  };
};

#endif // MUTEX_PRIVATE_H
