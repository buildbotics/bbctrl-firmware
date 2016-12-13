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

#include "Directory.h"

#define BOOST_SYSTEM_NO_DEPRECATED

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

using namespace std;
using namespace cb;


struct Directory::private_t {
  fs::path path;
  fs::directory_iterator it;

  private_t(const string &_path) :
    path(fs::system_complete(fs::path(_path))), it(path) {}
};


Directory::Directory(const string &path) : p(new private_t(path)) {
  if (!fs::is_directory(p->path)) THROWS("Not a directory '" << p->path << "'");
}


void Directory::rewind() {p->it = fs::directory_iterator(p->path);}
Directory::operator bool() const {return p->it != fs::directory_iterator();}
void Directory::next() {p->it++;}


const string Directory::getFilename() const {
#if BOOST_FILESYSTEM_VERSION < 3
  return p->it->path().filename();
#else
  return p->it->path().filename().string();
#endif
}


bool Directory::isSubdirectory() const {
  return fs::is_directory(p->it->status());
}
