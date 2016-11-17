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

#include "Module.h"

using namespace cb::chakra;
using namespace std;


Module::Module(const string &id, const string &path, JSImpl &impl) :
  Context(impl), obj(Value::createObject()) {
  obj.set("id", id);
  obj.setObject("exports");
  obj.set("filename", path);
  obj.set("name", id);
}


Value Module::load(const string &code) {
  enter();

  string path = obj.getString("filename");
  Value func =
    exec(path, "(function (id, require, exports, module) {" + code + "})");

  // Call
  vector<Value> args;
  args.push_back(obj.get("exports")); // Function "this"
  args.push_back(obj.get("id"));
  args.push_back(Value::getGlobal().get("require"));
  args.push_back(obj.get("exports"));
  args.push_back(obj);
  func.call(args);

  // Return exports
  return obj.get("exports");
}
