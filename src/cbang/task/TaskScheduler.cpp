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

#include "TaskScheduler.h"

#include <cbang/time/Timer.h>

#include <limits>

using namespace std;
using namespace cb;


void TaskScheduler::add(const SmartPointer<Task> &task, double nextRun) {
  if (tasks.has(task->getName()))
    THROWS("Already have task with name '" << task->getName() << "'");

  tasks.insert(task->getName(), TaskInfo(task, Timer::now() + nextRun));
}


const SmartPointer<Task> &TaskScheduler::get(const string &name) const {
  return tasks.get(name).task;
}


void TaskScheduler::set(const std::string &name, double nextRun) {
  tasks.get(name).nextRun = nextRun + (0 <= nextRun ? nextRun : 0);
}


void TaskScheduler::disable(const std::string &name) {
  tasks.get(name).nextRun = -1;
}


double TaskScheduler::update() {
  double now = Timer::now();
  double next = numeric_limits<double>::max();

  for (unsigned i = 0; i < tasks.size(); i++) {
    TaskInfo &info = tasks[i];

    if (0 <= info.nextRun && info.nextRun < now)
      info.nextRun += info.task->run();

    if (info.nextRun < next) next = info.nextRun;
  }

  return next;
}
