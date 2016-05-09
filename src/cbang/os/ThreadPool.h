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

#ifndef CBANG_THREAD_POOL_H
#define CBANG_THREAD_POOL_H

#include "Thread.h"

#include <cbang/SmartPointer.h>

#include <vector>


namespace cb {
  class ThreadPool {
    typedef std::vector<SmartPointer<Thread> > pool_t;
    pool_t pool;

  public:
    ThreadPool(unsigned size);
    virtual ~ThreadPool() {}

    void start();
    void stop();
    void join();
    void wait();

    void getStates(std::vector<Thread::state_t> &states) const;
    void getIDs(std::vector<unsigned> &ids) const;
    void getExitStatuses(std::vector<int> &statuses) const;

    void cancel();
    void kill(int signal);

  protected:
    /**
     * This function should be overriden by sub classes and will be called
     * by the running thread.
     */
    virtual void run() = 0;

    typedef pool_t::const_iterator iterator;
    iterator begin() const {return pool.begin();}
    iterator end() const {return pool.end();}
  };
}

#endif // CBANG_THREAD_POOL_H
