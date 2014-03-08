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

#ifndef CB_TASK_SCHEDULER_H
#define CB_TASK_SCHEDULER_H

#include "Task.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/OrderedDict.h>


namespace cb {
  class TaskScheduler {
    struct TaskInfo {
      SmartPointer<Task> task;
      double nextRun;

      TaskInfo(const SmartPointer<Task> &task, double nextRun) :
        task(task), nextRun(nextRun) {}
    };

    OrderedDict<TaskInfo> tasks;

  public:
    void add(const SmartPointer<Task> &task, double nextRun = 0);
    const SmartPointer<Task> &get(const std::string &name) const;
    void set(const std::string &name, double nextRun);
    void disable(const std::string &name);

    double update();
  };
}

#endif // CB_TASK_SCHEDULER_H

