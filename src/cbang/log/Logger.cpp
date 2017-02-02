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

#include "Logger.h"

#include "LogDevice.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/time/Time.h>
#include <cbang/iostream/NullDevice.h>

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/ThreadLocalStorage.h>

#include <cbang/config/Options.h>
#include <cbang/config/OptionActionSet.h>

#include <cbang/util/SmartLock.h>

#include <iostream>
#include <stdio.h> // for freopen()

#include <boost/ref.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
namespace io = boost::iostreams;

using namespace std;
using namespace cb;


Logger::Logger(Inaccessible) :
  verbosity(DEFAULT_VERBOSITY), logCRLF(false),
#ifdef DEBUG
  logDebug(true),
#else
  logDebug(false),
#endif
  logTime(true), logDate(false), logDatePeriodically(0), logShortLevel(false),
  logLevel(true), logThreadPrefix(false), logDomain(false),
  logSimpleDomains(true), logThreadID(false), logHeader(true),
  logNoInfoHeader(false), logColor(true), logToScreen(true), logTrunc(false),
  logRedirect(false), logRotate(true), logRotateMax(0), logRotateDir("logs"),
  threadIDStorage(new ThreadLocalStorage<unsigned long>),
  threadPrefixStorage(new ThreadLocalStorage<string>), screenStream(&cout),
  idWidth(1), lastDate(Time::now()) {

#ifdef _MSC_VER
  logCRLF = true;
  logColor = false; // Windows does not handle ANSI color codes well
#endif
}


void Logger::addOptions(Options &options) {
  options.pushCategory("Logging");
  options.add("log", "Set log file.");
  options.addTarget("verbosity", verbosity, "Set logging level for INFO "
#ifdef DEBUG
                    "and DEBUG "
#endif
                    "messages.");
  options.addTarget("log-crlf", logCRLF, "Print carriage return and line feed "
                    "at end of log lines.");
#ifdef DEBUG
  options.addTarget("log-debug", logDebug, "Disable or enable debugging info.");
#endif
  options.addTarget("log-time", logTime,
                    "Print time information with log entries.");
  options.addTarget("log-date", logDate,
                    "Print date information with log entries.");
  options.addTarget("log-date-periodically", logDatePeriodically,
                    "Print date to log before new log entries if so many "
                    "seconds have passed since the last date was printed.");
  options.addTarget("log-short-level", logShortLevel, "Print shortened level "
                    "information with log entries.");
  options.addTarget("log-level", logLevel,
                    "Print level information with log entries.");
  options.addTarget("log-thread-prefix", logThreadPrefix,
                    "Print thread prefixes, if set, with log entries.");
  options.addTarget("log-domain", logDomain,
                    "Print domain information with log entries.");
  options.addTarget("log-simple-domains", logSimpleDomains, "Remove any "
                    "leading directories and trailing file extensions from "
                    "domains so that source code file names can be easily used "
                    "as log domains.");
  options.add("log-domain-levels", 0,
              new OptionAction<Logger>(this, &Logger::domainLevelsAction),
              "Set log levels by domain.  Format is:\n"
              "\t<domain>[:i|d|t]:<level> ...\n"
              "Entries are separated by white-space and or commas.\n"
              "\ti - info\n"
#ifdef DEBUG
              "\td - debug\n"
#endif
#ifdef HAVE_DEBUGGER
              "\tt - enable traces\n"
#endif
              "For example: server:i:3 module:6\n"
              "Set 'server' domain info messages to level 3 and 'module' info "
#ifdef DEBUG
              "and debug "
#endif
              "messages to level 6.  All other domains will follow "
              "the system wide log verbosity level.\n"
              "If <level> is negative it is relative to the system wide "
              "verbosity."
              )->setType(Option::STRINGS_TYPE);
  options.addTarget("log-thread-id", logThreadID, "Print id with log entries.");
  options.addTarget("log-header", logHeader, "Enable log message headers.");
  options.addTarget("log-no-info-header", logNoInfoHeader,
                    "Don't print 'INFO(#):' in header.");
  options.addTarget("log-color", logColor,
                    "Print log messages with ANSI color coding.");
  options.addTarget("log-to-screen", logToScreen, "Log to screen.");
  options.addTarget("log-truncate", logTrunc, "Truncate log file.");
  options.addTarget("log-redirect", logRedirect, "Redirect all output to log "
                    "file.  Implies !log-to-screen.");
  options.addTarget("log-rotate", logRotate, "Rotate log files on each run.");
  options.addTarget("log-rotate-dir", logRotateDir,
                    "Put rotated logs in this directory.");
  options.addTarget("log-rotate-max", logRotateMax,
                    "Maximum number of rotated logs to keep.");
  options.popCategory();
}


void Logger::setOptions(Options &options) {
  if (options["log"].hasValue()) startLogFile(options["log"]);
}


int Logger::domainLevelsAction(Option &option) {
  setLogDomainLevels(option);
  return 0;
}


void Logger::startLogFile(const string &filename) {
  // Rotate log
  if (logRotate) SystemUtilities::rotate(filename, logRotateDir, logRotateMax);

  logFile = SystemUtilities::open(filename, ios::out |
                                  (logTrunc ? ios::trunc : ios::app));

  *logFile << String::bar(SSTR("Log Started " << Time()))
           << (logCRLF ? "\r\n" : "\n");
  logFile->flush();
  lastDate = Time::now();

  if (logRedirect) {
    setLogToScreen(false);

    SystemUtilities::open(filename, ios::app | ios::out); // Test filename

    // Redirect standard error and out
    if (!freopen(filename.c_str(), "a", stdout) ||
        !freopen(filename.c_str(), "a", stderr))
      THROWS("Redirecting output to '" << filename << "'");
  }
}


void Logger::setLogDomainLevels(const string &levels) {
  Option::strings_t entries;
  String::tokenize(levels, entries, Option::DEFAULT_DELIMS + ",");

  for (unsigned i = 0; i < entries.size(); i++) {
    vector<string> tokens;
    String::tokenize(entries[i], tokens, ":");
    bool invalid = false;

    if (tokens.size() == 3) {
      int level = String::parseS32(tokens[2]);

      for (unsigned j = 0; j < tokens[1].size(); j++)
        switch (tokens[1][j]) {
        case 'i': infoDomainLevels[tokens[0]] = level; break;
        case 'd': debugDomainLevels[tokens[0]] = level; break;
#ifdef HAVE_DEBUGGER
        case 't': domainTraces.insert(tokens[0]); break;
#endif
        default: invalid = true;
        }

    } else if (tokens.size() == 2) {
      int level = String::parseS32(tokens[1]);
      infoDomainLevels[tokens[0]] = level;
      debugDomainLevels[tokens[0]] = level;

    } else invalid = true;

    if (invalid) THROWS("Invalid log domain level entry " << (i + 1)
                        << " '" << entries[i] << "'");
  }
}


unsigned Logger::getHeaderWidth() const {
  return getHeader("", LOG_INFO_LEVEL(10)).size();
}


void Logger::setThreadID(unsigned long id) {
  threadIDStorage->set(id);
}


unsigned long Logger::getThreadID() const {
  return threadIDStorage->isSet() ? threadIDStorage->get() : 0;
}


void Logger::setThreadPrefix(const string &prefix) {
  threadPrefixStorage->set(prefix);
}


string Logger::getThreadPrefix() const {
  return threadPrefixStorage->isSet() ? threadPrefixStorage->get() : string();
}


string Logger::simplifyDomain(const string &domain) const {
  if (!logSimpleDomains) return domain;

  size_t pos = domain.find_last_of(SystemUtilities::path_separators);
  size_t start = pos == string::npos ? 0 : pos + 1;
  size_t end = domain.find_last_of('.');
  size_t len = end == string::npos ? end : end - start;

  return len ? domain.substr(start, len) : domain;
}


int Logger::domainVerbosity(const string &domain, int level) const {
  string dom = simplifyDomain(domain);

  domain_levels_t::const_iterator it;
  if ((level == LEVEL_DEBUG &&
       (it = debugDomainLevels.find(dom)) != debugDomainLevels.end()) ||
      (level == LEVEL_INFO &&
       (it = infoDomainLevels.find(dom)) != infoDomainLevels.end())) {
    int verbosity = it->second;

    // If verbosity < 0 then it is relative to Logger::verbosity
    if (verbosity < 0) verbosity += this->verbosity;
    return verbosity;
  }

  return verbosity;
}


string Logger::getHeader(const string &domain, int level) const {
  string header;

  if (!logHeader || level == LEVEL_RAW) return header;

  int verbosity = level >> 8;
  level &= LEVEL_MASK;

  // Thread ID
  if (logThreadID) {
    string idStr = String::printf("%0*ld:", idWidth - 1, getThreadID());
    if (idWidth < idStr.length()) {
      lock();
      idWidth = idStr.length();
      unlock();
    }

    header += idStr;
  }

  // Date & Time
  if (logDate || logTime) {
    uint64_t now = Time::now(); // Must be the same time for both
    if (logDate) header += Time(now, "%Y-%m-%d:").toString();
    if (logTime) header += Time(now, "%H:%M:%S:").toString();
  }

  // Level
  if (logShortLevel) {
    switch (level) {
    case LEVEL_ERROR:    header += "E"; break;
    case LEVEL_CRITICAL: header += "C"; break;
    case LEVEL_WARNING:  header += "W"; break;
    case LEVEL_INFO:     header += "I"; break;
    case LEVEL_DEBUG:    header += "D"; break;
    default: THROWS("Unknown log level " << level);
    }

    // Verbosity
    if (level >= LEVEL_INFO && verbosity) header += String(verbosity);
    else header += ' ';

    header += ':';

  } else if (logLevel) {
    switch (level) {
    case LEVEL_ERROR:    header += "ERROR"; break;
    case LEVEL_CRITICAL: header += "CRITICAL"; break;
    case LEVEL_WARNING:  header += "WARNING"; break;
    case LEVEL_INFO:     if (!logNoInfoHeader) header += "INFO"; break;
    case LEVEL_DEBUG:    header += "DEBUG"; break;
    default: THROWS("Unknown log level " << level);
    }

    if (!logNoInfoHeader || level != LEVEL_INFO) {
      // Verbosity
      if (level >= LEVEL_INFO && verbosity)
        header += string("(") + String(verbosity) + ")";

      header += ':';
    }
  }

  // Domain
  if (logDomain && domain != "") header += string(domain) + ':';

  // Thread Prefix
  if (logThreadPrefix) header += getThreadPrefix();

  return header;
}


const char *Logger::startColor(int level) const {
  if (!logColor) return "";

  switch (level & LEVEL_MASK) {
  case LEVEL_ERROR:    return "\033[91m";
  case LEVEL_CRITICAL: return "\033[31m";
  case LEVEL_WARNING:  return "\033[93m";
  case LEVEL_DEBUG:    return "\033[92m";
  default: return "";
  }
}


const char *Logger::endColor(int level) const {
  if (!logColor) return "";

  switch (level & LEVEL_MASK) {
  case LEVEL_ERROR:
  case LEVEL_CRITICAL:
  case LEVEL_WARNING:
  case LEVEL_DEBUG:
    return "\033[0m";
  default: return "";
  }
}


bool Logger::enabled(const string &domain, int level) const {
  unsigned verbosity = level >> 8;
  level &= LEVEL_MASK;

  if (!logDebug && level == LEVEL_DEBUG) return false;

  if (level >= LEVEL_INFO && domainVerbosity(domain, level) < (int)verbosity)
    return false;

  // All other log levels are always enabled
  return true;
}


Logger::LogStream Logger::createStream(const string &_domain, int level,
                                       const std::string &_prefix) {
  string domain = simplifyDomain(_domain);

  if (enabled(domain, level)) {
    // Log date periodically
    uint64_t now = Time::now();
    if (logDatePeriodically && lastDate + logDatePeriodically <= now) {
      write(String::bar(Time(now, "Date: %Y-%m-%d").toString()) +
            (logCRLF ? "\r\n" : "\n"));
      lastDate = now;
    }

    string prefix = startColor(level) + getHeader(domain, level) + _prefix;
    string suffix = endColor(level);

    string trailer;
#ifdef HAVE_DEBUGGER
    if (domainTraces.find(domain) != domainTraces.end()) {
      StackTrace trace;
      Debugger::instance().getStackTrace(trace);
      for (int i = 0; i < 3; i++) trace.pop_front();
      trailer = SSTR(trace);
    }
#endif

    return new cb::LogStream(prefix, suffix, trailer);

  } else return new NullStream<>;
}


streamsize Logger::write(const char *s, streamsize n) {
  if (!logFile.isNull()) logFile->write(s, n);
  if (logToScreen) screenStream->write(s, n);
  return n;
}


void Logger::write(const string &s) {
  write(s.c_str(), s.length());
}


bool Logger::flush() {
  if (!logFile.isNull()) logFile->flush();
  if (logToScreen) screenStream->flush();
  return true;
}
