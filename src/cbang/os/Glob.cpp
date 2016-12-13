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

#include "Glob.h"
#include <cbang/os/SystemUtilities.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#else
#include <glob.h>
#include <unistd.h>
#endif

#include <string.h>

using namespace std;
using namespace cb;


#ifdef _WIN32
string Glob::sep = "\\";
#else
string Glob::sep = "/";
#endif


namespace cb {
  struct glob_data_t {
#ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE f;
    bool next;
#else
    glob_t files;
    unsigned i;
#endif
  };
};


Glob::Glob(const string &pattern) : pattern(pattern), data(new glob_data_t) {
#ifdef _WIN32
  data->f = FindFirstFile(pattern.c_str(), &data->FindFileData);
  data->next = data->f != INVALID_HANDLE_VALUE;

#else
  memset(data, 0, sizeof(glob_data_t));
  glob(pattern.c_str(), 0, 0, &data->files);
#endif
}


Glob::~Glob() {
#ifdef _WIN32
  FindClose(data->f);

#else
  globfree(&data->files);
#endif

  delete data;
  data = 0;
}


bool Glob::hasNext() const {
#ifdef _WIN32
  return data->next;
#else
  return data->i < data->files.gl_pathc;
#endif
}


bool Glob::isDir() const {
#ifdef _WIN32
  return data->FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#else
  return SystemUtilities::isDirectory(data->files.gl_pathv[data->i]);
#endif
}


string Glob::next() {
#ifdef _WIN32
  string dir = SystemUtilities::dirname(pattern);
  string path = data->FindFileData.cFileName;

  data->next = FindNextFile(data->f, &data->FindFileData);

  return dir == "." ? path : SystemUtilities::joinPath(dir, path);

#else
  return data->files.gl_pathv[data->i++];
#endif
}
