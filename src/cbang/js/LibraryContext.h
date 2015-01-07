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

#ifndef CB_JS_LIBRARY_CONTEXT_H
#define CB_JS_LIBRARY_CONTEXT_H

#include "Value.h"
#include "Context.h"
#include "ObjectTemplate.h"

#include <cbang/SmartPointer.h>
#include <cbang/io/InputSource.h>

#include <ostream>
#include <map>
#include <vector>
#include <string>


namespace cb {
  namespace js {
    class LibraryContext : public ObjectTemplate {
      SmartPointer<Context> ctx;

      typedef std::map<std::string, PersistentValue> modules_t;
      modules_t modules;

      std::vector<std::string> pathStack;
      std::vector<std::string> searchExts;
      std::vector<std::string> searchPaths;

    public:
      std::ostream &out;

      LibraryContext(std::ostream &out);
      virtual ~LibraryContext() {}

      void pushPath(const std::string &path) {pathStack.push_back(path);}
      void popPath();
      const std::string &getCurrentPath() const;

      void addSearchExtensions(const std::string &exts);
      void appendSearchExtension(const std::string &ext);
      void clearSearchExtensions() {searchExts.clear();}
      std::vector<std::string> &getSearchExts() {return searchExts;}
      std::string searchExtensions(const std::string &path) const;

      void addSearchPaths(const std::string &paths);
      std::vector<std::string> &getSearchPaths() {return searchPaths;}
      void clearSearchPaths() {searchPaths.clear();}
      std::string searchPath(const std::string &path) const;

      virtual Value require(const std::string &path);
      virtual Value load(const std::string &path);
      virtual Value eval(const InputSource &source);
    };
  }
}

#endif // CB_JS_LIBRARY_CONTEXT_H

