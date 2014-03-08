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

#ifndef CB_JSON_SYNC_H
#define CB_JSON_SYNC_H

#include <cbang/Exception.h>

#include <string>


namespace cb {
  namespace JSON {
    class Value;

    class Sync {
    public:
      virtual ~Sync() {}

      // Element functions
      virtual void writeNull() {CBANG_THROW("Cannot write Null");}
      virtual void writeBoolean(bool value)
      {CBANG_THROW("Cannot write Boolean");}
      virtual void write(double value) {CBANG_THROW("Cannot write Number");}
      virtual void write(const std::string &value)
      {CBANG_THROW("Cannot write String");}

      // List functions
      virtual void beginList(bool simple = false)
      {CBANG_THROW("Cannot begin List");}
      virtual void beginAppend() {CBANG_THROW("Not a List");}
      virtual void endList() {CBANG_THROW("Not a List");}

      // Dict functions
      virtual void beginDict(bool simple = false)
      {CBANG_THROW("Cannot begin Dict");}
      virtual void beginInsert(const std::string &key)
      {CBANG_THROW("Not a Dict");}
      virtual void endDict() {CBANG_THROW("Not a Dict");}

      // List functions
      void appendNull() {beginAppend(); writeNull();}
      void appendBoolean(bool value) {beginAppend(); writeBoolean(value);}
      void append(double value) {beginAppend(); write(value);}
      void append(const std::string &value) {beginAppend(); write(value);}
      void appendList(bool simple = false) {beginAppend(); beginList(simple);}
      void appendDict(bool simple = false) {beginAppend(); beginDict(simple);}
      void append(const Value &value);

      // Dict functions
      void insertNull(const std::string &key) {beginInsert(key); writeNull();}
      void insertBoolean(const std::string &key, bool value)
      {beginInsert(key); writeBoolean(value);}
      void insert(const std::string &key, double value)
      {beginInsert(key); write(value);}
      virtual void insert(const std::string &key, const std::string &value)
      {beginInsert(key); write(value);}
      virtual void insertList(const std::string &key, bool simple = false)
      {beginInsert(key); beginList(simple);}
      virtual void insertDict(const std::string &key, bool simple = false)
      {beginInsert(key); beginDict(simple);}
      void insert(const std::string &key, const Value &value);
    };
  }
}

#endif // CB_JSON_SYNC_H

