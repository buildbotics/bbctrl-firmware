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

#ifndef CBANG_FILE_LOCATION_H
#define CBANG_FILE_LOCATION_H

#include <string>
#include <iostream>

namespace cb {

  /**
   * This class is mainly used by Exception, but can be used
   * as a general class for recording a line and column location
   * with in a file.
   */
  class FileLocation {
  protected:
    std::string filename;
    std::string function;
    long line;
    long col;
    bool empty;

  public:
    /**
     * Construct a default FileLocation with an empty value.
     */
    FileLocation() : line(-1), col(-1), empty(true) {}

    /**
     * Copy constructor.
     */
    FileLocation(const FileLocation &x) :
      filename(x.filename), function(x.function), line(x.line), col(x.col),
      empty(x.empty) {}

    /**
     * @param filename The name of the file.
     * @param line The line with that file.
     * @param col The column on that line.
     */
    FileLocation(const std::string &filename, const long line = -1,
                 const long col = -1) :
      filename(filename), line(line), col(col), empty(false) {}

    FileLocation(const std::string &filename, const std::string &function,
                 const long line = -1,  const long col = -1) :
      filename(filename), function(function), line(line), col(col),
      empty(false) {}

    virtual ~FileLocation() {}

    const std::string &getFilename() const {return filename;}
    void setFilename(const std::string &filename)
    {this->filename = filename; empty = false;}

    const std::string &getFunction() const {return function;}
    void setFunction(const std::string &function)
    {this->function = function; empty = false;}

    /// @return -1 if no line was set the line number otherwise
    long getLine() const {return line;}
    void setLine(long line) {this->line = line; empty = false;}
    void incLine() {line++; empty = false;}

    /// @return -1 of no column was set the column number otherwise
    long getCol() const {return col;}
    void setCol(long col) {this->col = col; empty = false;}
    void incCol() {col++; empty = false;}

    /// @return True of no filename, line, or column have been set
    bool isEmpty() const {return empty;}

    bool operator==(const FileLocation &o) const;
    bool operator!=(const FileLocation &o) const {return *this == o;}

    friend std::ostream &operator<<(std::ostream &stream,
                                    const FileLocation &fl);
  };

  /**
   * Print a file location to a stream.  The format is as follows.
   *
   * filename[:line[:col]]
   *
   * If no line or column has been set then they will not be displayed.
   *
   * @return A reference to the passed stream.
   */
  std::ostream &operator<<(std::ostream &stream, const FileLocation &fl);
}

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#ifdef DEBUG
#define CBANG_FILE_LOCATION cb::FileLocation(__FILE__, __func__, __LINE__, -1)

#else // DEBUG
#define CBANG_FILE_LOCATION cb::FileLocation()
#endif // DEBUG

#ifdef USING_CBANG
#define FILE_LOCATION CBANG_FILE_LOCATION
#endif

#endif // CBANG_FILE_LOCATION_H
