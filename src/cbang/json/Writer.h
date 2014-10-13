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

#ifndef CBANG_JSON_WRITER_H
#define CBANG_JSON_WRITER_H

#include "NullSync.h"

#include <ostream>


namespace cb {
  namespace JSON {
    class Writer : public NullSync {
      /***
       * The differences between JSON (Javascript Object Notation) and
       * JSON (Python Object Notation) are small.  They are as follows:
       *
       *                       |  JSON                |  JSON
       *-----------------------------------------------------------------
       * Boolean Literals      |  'true' & 'false'    |  'True' & 'False'
       * Empty set literal     |  'null'              |  'None'
       * Trailing comma        |  not allowed         |  allowed
       * Single quoted strings |  not allowed         |  allowed
       * Unquoted names        |  not allowed         |  allowed
       *
       * Python also allows other forms of escaped strings.
       * Also note that Javascript is more permissive than JSON.
       * See http://www.json.org/ for more info.
       */

    public:
      typedef enum {
        JSON_MODE,
        PYTHON_MODE,
      } mode_t;

    protected:
      std::ostream &stream;
      unsigned initLevel;
      unsigned level;
      bool compact;
      bool simple;
      mode_t mode;
      bool first;

    public:
      Writer(std::ostream &stream, unsigned indent = 0, bool compact = false,
             mode_t mode = JSON_MODE)
        : stream(stream), initLevel(indent), level(indent), compact(compact),
          simple(false), mode(mode), first(true) {}

      // From NullSync
      void close();
      void reset();

      // From Sync
      void writeNull();
      void writeBoolean(bool value);
      void write(double value);
      void write(const std::string &value);
      void beginList(bool simple = false);
      void beginAppend();
      void endList();
      void beginDict(bool simple = false);
      void beginInsert(const std::string &key);
      void endDict();

    protected:
      void indent() const {stream << std::string(level * 2, ' ');}
      bool isCompact() const {return compact || simple;}
    };
  }
}

#endif // CBANG_JSON_WRITER_H

