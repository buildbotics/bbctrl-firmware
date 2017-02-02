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

#ifndef CBANG_SYSTEM_UTILITIES_H
#define CBANG_SYSTEM_UTILITIES_H

#include <string>

#include <cbang/StdTypes.h>
#include <cbang/SmartPointer.h>

#include <cbang/util/StringMap.h>

#include <cbang/enum/ProcessPriority.h>

#include <limits>

#if defined(_MSC_VER) && defined(max)
#undef max
#endif


namespace cb {
  class URI;

  /// Contains system utility functions.  Most of these are OS dependent.
  namespace SystemUtilities {
    extern bool useHardLinks;

    extern const std::string path_separators;
    extern const char path_separator;
    extern const char path_delimiter;
    extern const std::string library_path;

    // TODO document these functions

    // Path
    std::string basename(const std::string &path);
    std::string dirname(const std::string &path);
    std::string::size_type getExtensionPosition(const std::string &path);
    bool hasExtension(const std::string &path);
    std::string extension(const std::string &path);
    std::string swapExtension(const std::string &path, const std::string &ext);
    std::vector<std::string> splitExt(const std::string &path);
    bool isAbsolute(const std::string &path);
    std::string relative(const std::string &base,
                         const std::string &target, unsigned maxDotDot =
                         std::numeric_limits<unsigned>::max());
    std::string absolute(const std::string &base, const std::string &target);
    std::string absolute(const std::string &path);
    std::string getCanonicalPath(const std::string &path);
    bool exists(const std::string &path);
    bool isFile(const std::string &path);
    void splitPath(const std::string &path, std::vector<std::string> &parts);
    void splitPaths(const std::string &s, std::vector<std::string> &paths);
    std::string joinPath(const std::string &left, const std::string &right);
    std::string joinPath(const std::vector<std::string> &parts);
    std::string joinPaths(const std::vector<std::string> &paths);
    std::string getExecutablePath();
    std::string findInPath(const std::string &path, const std::string &name);

    // Directory
    void mkdir(const std::string &path, bool withParents = true);
    void rmdir(const std::string &path, bool withChildren = false);
    bool ensureDirectory(const std::string &dir);
    bool isDirectory(const std::string &path);
    std::string getcwd();
    void chdir(const std::string &path);
    std::string createTempDir(const std::string &parent);

    // File
    uint64_t getFileSize(const std::string &filename);
    uint64_t getModificationTime(const std::string &filename);
    bool unlink(const std::string &filename);
    void symlink(const std::string &oldname, const std::string &newname);
    void link(const std::string &oldname, const std::string &newname);
    uint64_t cp(const std::string &src, const std::string &dst,
                uint64_t length = ~0);
    uint64_t cp(std::istream &in, std::ostream &out, uint64_t length = ~0);
    void rename(const std::string &src, const std::string &dst);
    SmartPointer<std::iostream>
    open(const std::string &filename,
         std::ios::openmode mode = std::ios::in | std::ios::out,
         int perm = 0644);
    SmartPointer<std::istream> iopen(const std::string &filename);
    SmartPointer<std::ostream>
    oopen(const std::string &filename, std::ios::openmode mode = std::ios::out,
          int perm = 0644);
    std::string read(const std::string &filename);
    void truncate(const std::string &path, unsigned long length);
    void chmod(const std::string &path, unsigned mode);
    void rotate(const std::string &path, const std::string &dir = std::string(),
                unsigned maxFiles = 0);
    int openModeToFlags(std::ios::openmode mode);


    // Process
    enum {
      PROCESS_SIGNALED    = 1 << 0,
      PROCESS_DUMPED_CORE = 1 << 1,
    };

    int priorityToInt(ProcessPriority priority);
    void setPriority(ProcessPriority priority, uint64_t pid = 0);
    int system(const std::string &cmd, const StringMap &env = StringMap());
    bool pidAlive(uint64_t pid);
    double getCPUTime();
    uint64_t getPID();
    bool waitPID(uint64_t pid, int *returnCode = 0, bool nonblocking = false,
                 int *flags = 0);
    bool killPID(uint64_t pid, bool group = false);
    void setUser(const std::string &user);
    void interruptProcessGroup();
    void daemonize();

    // User
    std::string getUserHome(const std::string &name);

    // Environment
    const char *getenv(const std::string &name);
    void setenv(const std::string &name, const std::string &value);
    void unsetenv(const std::string &name);
    void clearenv();

    // Interaction
    std::string readline(std::istream &in, std::ostream &out,
                         const std::string &message,
                         const std::string &defaultValue = std::string(),
                         const std::string &suffix = ": ");

    // Network
    void openURI(const URI &uri);
  }
}

#endif // CBANG_SYSTEM_UTILITIES_H
