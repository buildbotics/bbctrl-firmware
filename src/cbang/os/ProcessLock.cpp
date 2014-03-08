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

#include "ProcessLock.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/time/Time.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef _WIN32
#include <io.h>
#define O_CREAT _O_CREAT
#define O_WRONLY _O_WRONLY
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


using namespace cb;
using namespace std;

namespace ip = boost::interprocess;
namespace pt = boost::posix_time;


struct ProcessLock::private_t {
  ip::file_lock *lock;
  SmartPointer<iostream> f;

  private_t() : lock(0) {}

  ~private_t() {
    if (lock) delete lock;
    lock = 0;
  }
};


ProcessLock::ProcessLock(const string &filename, unsigned wait) :
  filename(filename), p(new private_t) {

  // Ensure lock file exists
  // NOTE: Must use open() here because there is no other way to create the
  // file if it does not exist with out truncating as well.
  int fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0666);
  if (fd == -1)
    THROWS("Failed to access process lock file '" << filename << "': "
           << SysError());
  close(fd);

  p->lock = new ip::file_lock(filename.c_str());

  bool locked;
  if (wait) locked = p->lock->timed_lock(pt::from_time_t(Time::now() + wait));
  else locked = p->lock->try_lock();

  if (locked) {
    // Keep file open to avoid losing the file lock
    p->f = SystemUtilities::open(filename, ios::out | ios::trunc);
    *p->f << SystemUtilities::getPID() << endl;

  } else {
    delete p->lock;
    p->lock = 0;

    char pid[1024] = {0};
    SystemUtilities::open(filename, ios::in)->getline(pid, 1024);

    THROWS("Cannot get an exclusive lock on '" << filename
           << "'.  Another process must be running with PID="
           << (pid[0] ? pid : "unknown"));
  }
}


ProcessLock::~ProcessLock() {
  if (p) delete p;
  p = 0;
}
