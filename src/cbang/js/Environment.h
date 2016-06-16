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

#ifndef CB_JS_ENVIRONMENT_H
#define CB_JS_ENVIRONMENT_H

#include "Value.h"
#include "Context.h"
#include "ObjectTemplate.h"
#include "ConsoleModule.h"

#include <cbang/SmartPointer.h>
#include <cbang/io/InputSource.h>

#include <ostream>
#include <map>
#include <vector>
#include <string>


namespace cb {
  namespace js {
    class Module;


    class Environment : public ObjectTemplate {
      SmartPointer<Context> ctx;

      typedef std::map<std::string, PersistentValue> modules_t;
      modules_t modules;

      ConsoleModule consoleMod;

      std::vector<std::string> pathStack;
      std::vector<std::string> searchExts;
      std::vector<std::string> searchPaths;

      std::ostream &out;

    public:
      Environment(std::ostream &out);
      virtual ~Environment() {}

      virtual void pushPath(const std::string &path);
      virtual void popPath();
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

      virtual void addModule(const std::string &name, const Value &exports);
      virtual void addModule(const std::string &name, Module &module);

      virtual Value require(const std::string &path);
      virtual Value load(Context &ctx, const std::string &path);
      virtual Value load(const std::string &path);
      virtual Value eval(Context &ctx, const InputSource &source);
      virtual Value eval(const InputSource &source);

      virtual Value require(const Arguments &args);
      virtual void print(const Arguments &args);
      virtual void alert(const Arguments &args);
    };
  }
}

#endif // CB_JS_ENVIRONMENT_H
