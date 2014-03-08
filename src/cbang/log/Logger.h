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

#ifndef CBANG_LOGGER_H
#define CBANG_LOGGER_H

#include <ostream>
#include <string>
#include <map>
#include <set>

#include <cbang/SStream.h>
#include <cbang/SmartPointer.h>
#include <cbang/Exception.h>

#include <cbang/util/Singleton.h>

#include <cbang/os/Mutex.h>

#ifdef DEBUG_LEVEL
#define DEFAULT_VERBOSITY DEBUG_LEVEL
#else
#define DEFAULT_VERBOSITY 1
#endif

namespace cb {
  class Option;
  class Options;
  class CommandLine;
  template <typename T> class ThreadLocalStorage;

  /**
   * Handles all information logging.  Both to the screen and optionally to
   * a log file.
   * Logging should be done through the provided macros e.g. LOG_ERROR
   * The logging macros allow C++ iostreaming.  For example.
   *   LOG_INFO(3, "The value was " << x << ".");
   */
  class Logger : public Mutex, public Singleton<Logger> {
  public:
    enum log_level_t {
      LEVEL_RAW      = 0,
      LEVEL_ERROR    = 1 << 2,
      LEVEL_CRITICAL = 1 << 3,
      LEVEL_WARNING  = 1 << 4,
      LEVEL_INFO     = 1 << 5,
      LEVEL_DEBUG    = 1 << 6,
      LEVEL_MASK     = (1 << 7) - 4
    };

  private:
    unsigned verbosity;
    bool logCRLF;
    bool logDebug;
    bool logTime;
    bool logDate;
    uint64_t logDatePeriodically;
    bool logShortLevel;
    bool logLevel;
    bool logThreadPrefix;
    bool logDomain;
    bool logSimpleDomains;
    bool logThreadID;
    bool logHeader;
    bool logNoInfoHeader;
    bool logColor;
    bool logToScreen;
    bool logTrunc;
    bool logRedirect;
    bool logRotate;
    unsigned logRotateMax;
    std::string logRotateDir;

    SmartPointer<ThreadLocalStorage<unsigned long> > threadIDStorage;
    SmartPointer<ThreadLocalStorage<std::string> > threadPrefixStorage;

    SmartPointer<std::iostream> logFile;
    std::ostream *screenStream;

    mutable unsigned idWidth;

    typedef std::map<std::string, int> domain_levels_t;
    domain_levels_t infoDomainLevels;
    domain_levels_t debugDomainLevels;

#ifdef HAVE_DEBUGGER
    typedef std::set<std::string> domain_traces_t;
    domain_traces_t domainTraces;
#endif

    uint64_t lastDate;

  public:
    Logger(Inaccessible);

    void addOptions(Options &options);
    void setOptions(Options &options);
    int domainLevelsAction(Option &option);

    /**
     * Begin logging to a file.
     * @param filename The log file name.
     */
    void startLogFile(const std::string &filename);
    bool getLogFileStarted() const {return logFile.get();}

    void setScreenStream(std::ostream &stream) {screenStream = &stream;}

    /**
     * Set the logging verbosity level.
     * @param verbosity The level.
     */
    void setVerbosity(unsigned x) {verbosity = x;}
    void setLogDebug(bool x) {logDebug = x;}
    void setLogCRLF(bool x) {logCRLF = x;}
    void setLogTime(bool x) {logTime = x;}
    void setLogDate(bool x) {logDate = x;}
    void setLogShortLevel(bool x) {logShortLevel = x;}
    void setLogLevel(bool x) {logLevel = x;}
    void setLogThreadPrefix(bool x) {logThreadPrefix = x;}
    void setLogDomain(bool x) {logDomain = x;}
    void setLogSimpleDomains(bool x) {logSimpleDomains = x;}
    void setLogThreadID(bool x) {logThreadID = x;}
    void setLogNoInfoHeader(bool x) {logNoInfoHeader = x;}
    void setLogHeader(bool x) {logHeader = x;}
    void setLogColor(bool x) {logColor = x;}
    void setLogToScreen(bool x) {logToScreen = x;}
    void setLogTruncate(bool x) {logTrunc = x;}
    void setLogRedirect(bool x) {logRedirect = x;}
    void setLogRotate(bool x) {logRotate = x;}
    void setLogRotateMax(unsigned x) {logRotateMax = x;}
    void setLogDomainLevels(const std::string &levels);

    unsigned getVerbosity() const {return verbosity;}
    bool getLogCRLF() const {return logCRLF;}
    unsigned getHeaderWidth() const;

    void setThreadID(unsigned long id);
    unsigned long getThreadID() const;
    void setThreadPrefix(const std::string &prefix);
    std::string getThreadPrefix() const;

    std::string simplifyDomain(const std::string &domain) const;
    int domainVerbosity(const std::string &domain, int level) const;
    std::string getHeader(const std::string &domain, int level) const;
    const char *startColor(int level) const;
    const char *endColor(int level) const;

    // These functions should not be called directly.  Use the macros.
    bool enabled(const std::string &domain, int level) const;
    typedef SmartPointer<std::ostream> LogStream;
    LogStream createStream(const std::string &domain, int level,
                           const std::string &prefix = std::string());

  protected:
    std::streamsize write(const char *s, std::streamsize n);
    void write(const std::string &s);
    bool flush();

    friend class LogDevice;
  };
}

#ifndef CBANG_LOG_DOMAIN
#define CBANG_LOG_DOMAIN __FILE__
#endif

// Compute DEBUG and INFO levels
#define CBANG_LOG_DEBUG_LEVEL(x) (cb::Logger::LEVEL_DEBUG + ((x) << 8))
#define CBANG_LOG_INFO_LEVEL(x)  (cb::Logger::LEVEL_INFO + ((x) << 8))


// Check if logging level is enabled
#define CBANG_LOG_ENABLED(domain, level)        \
  cb::Logger::instance().enabled(domain, level)
#ifdef DEBUG
#define CBANG_LOG_DEBUG_ENABLED(x)                              \
  CBANG_LOG_ENABLED(CBANG_LOG_DOMAIN, CBANG_LOG_DEBUG_LEVEL(x))
#else
#define CBANG_LOG_DEBUG_ENABLED(x) false
#endif
#define CBANG_LOG_INFO_ENABLED(x)                               \
  CBANG_LOG_ENABLED(CBANG_LOG_DOMAIN, CBANG_LOG_INFO_LEVEL(x))


// Create logger streams
// Warning these macros lock the Logger until they are deallocated
#define CBANG_LOG_STREAM(domain, level)                 \
  cb::Logger::instance().createStream(domain, level)

#define CBANG_LOG_RAW_STREAM()                              \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_RAW)
#define CBANG_LOG_ERROR_STREAM()                                \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_ERROR)
#define CBANG_LOG_CRITICAL_STREAM()                                 \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_CRITICAL)
#define CBANG_LOG_WARNING_STREAM()                              \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_WARNING)
#define CBANG_LOG_DEBUG_STREAM(level)                               \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_DEBUG_LEVEL(level))
#define CBANG_LOG_INFO_STREAM(level)                                \
  CBANG_LOG_STREAM(CBANG_LOG_DOMAIN, CBANG_LOG_INFO_LEVEL(level))


// Log messages
// The 'do {...} while (false)' loop lets this compile correctly:
//   if (expr) CBANG_LOG(...); else ...
#define CBANG_LOG(domain, level, msg)           \
  do {                                          \
    if (CBANG_LOG_ENABLED(domain, level))       \
      *CBANG_LOG_STREAM(domain, level) << msg;  \
  } while (false)

#define CBANG_LOG_RAW(msg)                                  \
  CBANG_LOG(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_RAW, msg)
#define CBANG_LOG_ERROR(msg)                                \
  CBANG_LOG(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_ERROR, msg)
#define CBANG_LOG_CRITICAL(msg)                                 \
  CBANG_LOG(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_CRITICAL, msg)
#define CBANG_LOG_WARNING(msg)                                  \
  CBANG_LOG(CBANG_LOG_DOMAIN, cb::Logger::LEVEL_WARNING, msg)
#define CBANG_LOG_INFO(x, msg)                              \
  CBANG_LOG(CBANG_LOG_DOMAIN, CBANG_LOG_INFO_LEVEL(x), msg)

#ifdef DEBUG
#define CBANG_LOG_DEBUG(x, msg)                                 \
  CBANG_LOG(CBANG_LOG_DOMAIN, CBANG_LOG_DEBUG_LEVEL(x), msg)
#else
#define CBANG_LOG_DEBUG(x, msg)
#endif


#ifdef USING_CBANG
#define LOG_DEBUG_LEVEL(x) CBANG_LOG_DEBUG_LEVEL(x)
#define LOG_INFO_LEVEL(x) CBANG_LOG_INFO_LEVEL(x)

#define LOG_ENABLED(domain, level) CBANG_LOG_ENABLED(domain, level)
#define LOG_DEBUG_ENABLED(x) CBANG_LOG_DEBUG_ENABLED(x)
#define LOG_INFO_ENABLED(x) CBANG_LOG_INFO_ENABLED(x)

#define LOG_STREAM(domain, level) CBANG_LOG_STREAM(domain, level)
#define LOG_RAW_STREAM() CBANG_LOG_RAW_STREAM()
#define LOG_ERROR_STREAM() CBANG_LOG_ERROR_STREAM()
#define LOG_CRITICAL_STREAM() CBANG_LOG_CRITICAL_STREAM()
#define LOG_WARNING_STREAM() CBANG_LOG_WARNING_STREAM()
#define LOG_DEBUG_STREAM(level) CBANG_LOG_DEBUG_STREAM(level)
#define LOG_INFO_STREAM(level) CBANG_LOG_INFO_STREAM(level)

#define LOG(domain, level, msg) CBANG_LOG(domain, level, msg)
#define LOG_RAW(msg) CBANG_LOG_RAW(msg)
#define LOG_ERROR(msg) CBANG_LOG_ERROR(msg)
#define LOG_CRITICAL(msg) CBANG_LOG_CRITICAL(msg)
#define LOG_WARNING(msg) CBANG_LOG_WARNING(msg)
#define LOG_INFO(x, msg) CBANG_LOG_INFO(x, msg)
#define LOG_DEBUG(x, msg) CBANG_LOG_DEBUG(x, msg)
#endif // USING_CBANG

#endif // CBANG_LOGGER_H
