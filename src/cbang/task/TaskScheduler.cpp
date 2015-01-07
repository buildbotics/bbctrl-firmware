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

#include "TaskScheduler.h"

#include <cbang/time/Timer.h>
#include <cbang/util/DefaultCatch.h>

#include <limits>

using namespace std;
using namespace cb;


void TaskScheduler::add(const SmartPointer<Task> &task) {
  if (tasks.has(task->getName()))
    THROWS("Already have task with name '" << task->getName() << "'");

  tasks.insert(task->getName(), task);
}


const SmartPointer<Task> &TaskScheduler::get(const string &name) const {
  return tasks.get(name);
}


double TaskScheduler::schedule() {
  double next = numeric_limits<double>::max();

  for (unsigned i = 0; i < tasks.size(); i++) {
    Task &task = *tasks[i];
    double now = Timer::now();

    if (0 <= task.getNextRun() && task.getNextRun() < now) {
      task.setLastRun(now);

      try {
        task.run();
      } CATCH_ERROR;
    }

    if (task.getNextRun() < next) next = task.getNextRun();
  }

  return next;
}
