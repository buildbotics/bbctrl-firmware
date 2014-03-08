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

#include "UnixFile.h"

#include <cbang/Exception.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

#define mode_t int
#define ssize_t SSIZE_T

#else
#include <unistd.h>
#endif

using namespace std;
using namespace cb;

#define IS_SET(x, y) (((x) & (y)) == (y))


UnixFile::UnixFile(const string &path, ios::openmode mode, int perm) : fd(-1) {
  open(path, mode, perm);
}


void UnixFile::open(const string &path, ios::openmode mode, int perm) {
  SysError::clear();

  if (is_open()) BOOST_IOS_THROWS("File already open");

#ifdef _WIN32
  perm &= 0600; // Windows only understands these permissions.
#endif

  fd = _open(path, mode, perm);

  if (!is_open()) BOOST_IOS_THROWS("Failed to open '" << path << "'");

  // Simulate ios::ate
  if (IS_SET(mode, ios::ate)) seek(0, ios::end);
}


streamsize UnixFile::read(char *s, streamsize n) {
  if (!is_open()) return -1;
  if (!n) return 0;

  SysError::clear();
  streamsize result = _read(fd, s, n);

  if (SysError::get()) {
    BOOST_IOS_THROWS("read() failed");
    return -1;
  }

  return result == 0 ? -1 : result;
}


streamsize UnixFile::write(const char *s, streamsize n) {
  if (!is_open()) return -1;
  if (!n) return 0;

  SysError::clear();
  ssize_t size;
  size = _write(fd, s, n);

  if (size < 0) BOOST_IOS_THROWS("write() failed");

  return size;
}


streampos UnixFile::seek(streampos off, ios::seekdir way) {
  if (!is_open()) return -1;
  int whence = 0;

  SysError::clear();

  switch (way) {
  case ios::beg: whence = SEEK_SET; break;
  case ios::cur: whence = SEEK_CUR; break;
  case ios::end: whence = SEEK_END; break;
  default:
    BOOST_IOS_THROWS("Invalid seek()");
    return -1;
  }

  off_t o = lseek(fd, off, whence);
  if (o == (off_t)-1) BOOST_IOS_THROWS("seek() failed");

  return o;
}


void UnixFile::close() {
  if (is_open()) {
    _close(fd);
    fd = -1;
  }
}


streamsize UnixFile::size() const {
  struct stat buf;

  if (fstat(fd, &buf) == -1)
    THROWS("Error getting file status: " << SysError());

  return buf.st_size;
}


int UnixFile::_open(const string &path, ios::openmode mode, int perm) {
  return ::open(path.c_str(), openModeToFlags(mode), (mode_t)perm);
}


streamsize UnixFile::_read(int fd, char *s, streamsize n) {
  return ::read(fd, s, n);
}


streamsize UnixFile::_write(int fd, const char *s, streamsize n) {
  return ::write(fd, s, n);
}


void UnixFile::_close(int fd) {
  ::close(fd);
}


int UnixFile::openModeToFlags(ios::openmode mode) {
  int flags = 0;

  if (IS_SET(mode, ios::in | ios::out)) flags |= O_RDWR;
  else if (IS_SET(mode, ios::in)) flags |= O_RDONLY;
  else if (IS_SET(mode, ios::out)) {
    flags |= O_WRONLY;
    if (!(IS_SET(mode, ios::ate) || IS_SET(mode, ios::app)))
      flags |= O_TRUNC;
  }

  if (IS_SET(mode, ios::out)) {
    if (IS_SET(mode, ios::trunc)) flags |= O_TRUNC;
    if (IS_SET(mode, ios::app)) flags |= O_APPEND;
    flags |= O_CREAT; // The default for ios::out
  }

#ifdef _WIN32
  flags |= O_BINARY; // Always use binary mode
  flags |= O_NOINHERIT; // Don't inherit handles by default
#endif

#ifdef _LARGEFILE64_SOURCE
  flags |= O_LARGEFILE;
#endif

  return flags;
}
