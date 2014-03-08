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

#ifndef CBANG_DIRECTORY_WALKER_H
#define CBANG_DIRECTORY_WALKER_H

#include <vector>
#include <string>

#include <cbang/SmartPointer.h>
#include <cbang/os/Directory.h>

#include <boost/regex.hpp>

namespace cb {
  /// Walk a directory tree and return files that match pattern.
  class DirectoryWalker {
    boost::regex re;
    std::string path;
    std::vector<SmartPointer<Directory> > dirStack;
    std::string nextFile;
    unsigned maxDepth;

  public:
    /**
     * Create a DirectoryWalker.
     * A maxDepth of ~0 will seach indefinately deep.
     * A maxDepth of 0 or 1 will only search the root.
     *
     * @param root The root of the directory tree to search.
     * @param pattern The file regular expression to match
     * @param maxDepth The maximum directory depth.
     */
    DirectoryWalker(const std::string &root = "",
                    const std::string &pattern = ".*",
                    unsigned maxDepth = ~0);

    /**
     * Initialize the DirectoryWalker with a new root and start over.
     *
     * @param root The path to the root directory.
     */
    void init(const std::string &root);

    /// @return True if there is another matching file available.
    bool hasNext();

    /// @return The full path name of the next matching file.
    const std::string next();

  protected:
    void push(const std::string &dir);
    void pop();
  };
}

#endif // CBANG_DIRECTORY_WALKER_H

