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

#ifndef CB_CHAKRA_SINK_H
#define CB_CHAKRA_SINK_H

#include "Value.h"

#include <cbang/js/Sink.h>


namespace cb {
  namespace chakra {
    class Sink : public js::Sink {
      Value root;

      bool closeList;
      bool closeDict;

      int index;
      std::string key;
      std::vector<Value> stack;

    public:
      Sink(const Value &root = Value::getUndefined());

      Value getRoot() const {return root;}

      // From JSON::NullSink
      void close();
      void reset();

      // Element functions
      void writeNull();
      void writeBoolean(bool value);
      void write(double value);
      void write(const std::string &value);
      void write(const js::Function &func);
      void write(const Value &value);

      // List functions
      void beginList(bool simple = false);
      void beginAppend();
      void endList();

      // Dict functions
      void beginDict(bool simple = false);
      void beginInsert(const std::string &key);
      void endDict();
   };
  }
}

#endif // CB_CHAKRA_SINK_H
