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

#include "Application.h"

#include <cbang/Zap.h>
#include <cbang/Info.h>

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SystemInfo.h>
#include <cbang/os/SignalManager.h>

#include <cbang/log/Logger.h>

#include <cbang/time/Timer.h>
#include <cbang/time/Time.h>
#include <cbang/time/HumanTime.h>

#include <cbang/util/DefaultCatch.h>
#include <cbang/util/Callback.h>

#include <cbang/config/Option.h>

#include <cbang/pyon/Message.h>

#include <cbang/json/Dict.h>

#include <cbang/xml/XMLWriter.h>

#include <cbang/socket/SocketDebugger.h>

#include <cbang/script/MemberFunctor.h>

#include <cbang/enum/EnumerationManager.h>

#include <sstream>
#include <set>

#ifndef _WIN32
#include <sys/resource.h>
#endif


using namespace std;
using namespace cb;
using namespace cb::Script;


namespace cb {
  namespace BuildInfo {
    void addBuildInfo(const char *category);
  }
}


Application::Application(const string &name, hasFeature_t hasFeature) :
  Features(hasFeature), Environment(name), logger(Logger::instance()),
  enumMan(new EnumerationManager(*this)), name(name), configRotate(true),
  configRotateMax(16), configRotateDir("configs"), initialized(false),
  configured(false), quit(false), startTime(Timer::now()) {

  // Core dumps
#if defined(DEBUG) && !defined(_WIN32)
  // Enable full core dumps in debug mode
  struct rlimit rlim;
  rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &rlim);
#endif

#ifndef _WIN32
  // Ignore SIGPIPE by default
  if (hasFeature(FEATURE_SIGNAL_HANDLER))
    SignalManager::instance().ignoreSignal(SIGPIPE);
#endif

#ifndef DEBUG
  // Hide debugging options in non-debug builds
  options.getCategory("Debugging")->setHidden(true);
#endif

  // Options
  if (hasFeature(FEATURE_DEBUGGING)) {
    options.pushCategory("Debugging");
    options.addTarget("stack-traces", Exception::enableStackTraces,
                      "Enable or disable stack traces on errors.");
    options.addTarget("exception-locations", Exception::printLocations,
                      "Enable or disable exception location printing.");
    options.popCategory();

    SocketDebugger::instance().addOptions(options);
  }

  if (hasFeature(FEATURE_PROCESS_CONTROL)) {
    options.pushCategory("Process Control");
    options.add("service", "Ignore user logout or hangup and interrupt signals"
                )->setDefault(false);
    options.add("priority", "Set the process priority. Valid values are: idle, "
                "low, normal, high or realtime.");
    options.alias("priority", "nice");
    options.popCategory();
  }

  if (hasFeature(FEATURE_CONFIG_FILE) && hasFeature(FEATURE_SCRIPT_SERVER)) {
    options.pushCategory("Configuration");
    options.addTarget("config-rotate", configRotate, "Rotate the "
                      "configuration file to a backup when saving would "
                      "overwrite the previous configuration.");
    options.addTarget("config-rotate-dir", configRotateDir,
                      "Put rotated configs in this directory.");
    options.addTarget("config-rotate-max", configRotateMax, "The maximum "
                      "number of rotated configuration files to keep.  A "
                      "value of zero will keep all configuration file "
                      "backups.");
    options.popCategory();
  }

  logger.addOptions(options);

  // Command line
  if (hasFeature(FEATURE_CONFIG_FILE)) {
    cmdLine.pushCategory("Configuration");

    cmdLine.add("config", 0, this, &Application::configAction,
                "Set configuration file.")->setDefault("config.xml");

    cmdLine.add("print", 0, this, &Application::printAction,
                "Print configuration and exit.");

    cmdLine.popCategory();
  }

  cmdLine.pushCategory("Informational");

  cmdLine.add("version", 0, this, &Application::versionAction,
              "Print application version and exit.");

  if (hasFeature(FEATURE_INFO))
    cmdLine.add("info", 0, this, &Application::infoAction,
                "Print application and system information and exit.");

  cmdLine.popCategory();
  cmdLine.setKeywordOptions(&options);

  // Info
  if (hasFeature(FEATURE_INFO)) {
    Info &info = Info::instance();
    BuildInfo::addBuildInfo("Build");
    SystemInfo::instance().add(info);

    // Add to info
    info.add("System", "UTC Offset", String(Time::offset() / 3600));
    info.add("System", "PID", String(SystemUtilities::getPID()));
    info.add("System", "CWD", SystemUtilities::getcwd());
  }

  // Script functions
  typedef Application A;
  typedef Script::MemberFunctor<A> MF;

  add(new MF("uptime", this, &A::evalUptime, 0, 0,
             "Print application uptime"));
  add(new MF("option", this, &A::evalOption, 1, 2,
             "Get or set a configuration option", "<name> [value]"));

  if (hasFeature(FEATURE_SCRIPT_SERVER)) {
    if (hasFeature(FEATURE_INFO)) {
      add(new MF("get-info", this, &A::evalGetInfo, 2, 2,
                 "Print application information", "<category> <key>"));
      add(new MF("info", this, &A::evalInfo, 0, 0,
                 "Print application information in PyON format"));
    }
    add(new MF("options", this, &A::evalOptions, 0, ~0,
               "List or set options with their values.\n"
               "If no name arguments are given then all options with "
               "non-default values will be listed.  If the '-d' argument is "
               "given then even defaulted options will be listed.  If the "
               "'-a' option is given then unset options will also be listed.  "
               "Otherwise, if option names are provided only those options "
               "will be listed.\n"
               "The special name '*' lists all options which have not yet been "
               "listed and is affected by the '-d' and '-a' options.\n"
               "If a name argument is followed directly by an equal sign then "
               "the rest of the arugment will be used to set the option's "
               "value.  If instead a name argument is followed immediately by "
               "a '!' then the option will be reset to its default value.\n"
               "Options which are set or reset will also be listed.\n"
               "Options are listed as a PyON format dictionary."
               "[-d | -a] | [<name>[! | =<value>]]..."));
    add(new MF("shutdown", this, &A::evalShutdown, 0, 0,
               "Shutdown the application"));
    add(new MF("save", this, &A::evalSave, 0, 1,
               "Save the configuration either to the specified file or to the "
               "file the configuration was last loaded from.", "[file]"));

    if (hasFeature(FEATURE_DEBUGGING))
      SocketDebugger::instance().addCommands(*this);
  }
}


Application::~Application() {
  zap(enumMan);

#ifdef DEBUG_LEAKS
  // Deallocate singletons
  SingletonDealloc::instance().deallocate();
#endif // DEBUG_LEAKS
}


bool Application::_hasFeature(int feature) {
  switch (feature) {
  case FEATURE_CONFIG_FILE:
  case FEATURE_DEBUGGING:
  case FEATURE_PRINT_INFO:
  case FEATURE_SIGNAL_HANDLER:
    return true;

  default: return false;
  }
}


double Application::getUptime() const {
  return Timer::now() - startTime;
}


int Application::init(int argc, char *argv[]) {
  if (initialized) THROW("Already initialized");
  initialized = true;
  quit = false;

  if (hasFeature(FEATURE_INFO))
    version = Version(Info::instance().get("Build", "Version"));

  // Add args to info, obscuring any obscured options
  if (hasFeature(FEATURE_INFO)) {
    Info &info = Info::instance();
    ostringstream args;
    bool obscureNext = false;
    for (int i = 1; i < argc; i++) {
      if (i) args << " ";

      string arg = argv[i];
      if (2 < arg.length() && arg.substr(0, 2) == "--") {
        string name = arg.substr(2);
        size_t equal = name.find('=');
        if (equal != string::npos) name = name.substr(0, equal);

        if (options.has(name) && options[name].isObscured()) {
          if (equal == string::npos) obscureNext = true;
          else arg = arg.substr(0, equal + 3) +
                 string(arg.length() - equal - 3, '*');
        }

      } else if (obscureNext) {
        arg = string(arg.length(), '*');
        obscureNext = false;
      }

      args << arg;
    }
    info.add(name, "Args", args.str());
  }

  // Parse args
  int ret = cmdLine.parse(argc, argv);
  if (ret == -1) return -1;

  // Load default config
  if (hasFeature(FEATURE_CONFIG_FILE)) {
    if (!configured) {
      if (cmdLine["--config"].hasValue() &&
          SystemUtilities::exists(cmdLine["--config"])) {
        configAction(cmdLine["--config"]);
      } else Info::instance().add(name, "Config", "<none>");
    }
  }

  logger.setOptions(options);
  LOG_DEBUG(3, "Initializing " << name);

  if (hasFeature(FEATURE_PROCESS_CONTROL))
    try {
      if (options["priority"].hasValue())
        SystemUtilities::setPriority(ProcessPriority::parse
                                     (options["priority"]));
    } CBANG_CATCH_WARNING;

  if (hasFeature(FEATURE_SIGNAL_HANDLER))
    catchExitSignals(); // Also enables SignalManager

  if (hasFeature(FEATURE_PRINT_INFO)) printInfo();

  initialize();
  return ret;
}


void Application::printInfo() const {
  // Print Info
  if (hasFeature(FEATURE_INFO))
    Info::instance().print
      (*LOG_INFO_STREAM(1), 80 - Logger::instance().getHeaderWidth());

  // Write config to log
  if (hasFeature(FEATURE_CONFIG_FILE))
    writeConfig(*LOG_INFO_STREAM(2), 3 < Logger::instance().getVerbosity() ?
                Option::DEFAULT_SET_FLAG : 0);
}


void Application::write(XMLWriter &writer, uint32_t flags) const {
  getOptions().write(writer, flags);
}


ostream &Application::print(ostream &stream) const {
  return stream << options;
}


void Application::usage(ostream &stream, const string &name) const {
  cmdLine.usage(stream, name);
}


void Application::openConfig(const string &_filename) {
  string filename;
  if (_filename.empty()) filename = cmdLine["--config"];
  else filename = _filename;

  configReader.read(filename, &options);
  configured = true;

  // Add config to info
  Info::instance().add(name, "Config", SystemUtilities::absolute(filename));
}


void Application::saveConfig(const string &_filename) const {
  string filename;
  if (_filename.empty()) filename = cmdLine["--config"];
  else filename = _filename;

  if (configRotate)
    SystemUtilities::rotate(filename, configRotateDir, configRotateMax);

  LOG_INFO(1, "Saving configuration to " << filename);
  writeConfig(*LOG_INFO_STREAM(2));
  writeConfig(*SystemUtilities::open(filename, ios::out),
              Option::OBSCURED_FLAG);
}


void Application::writeConfig(ostream &stream, uint32_t flags) const {
  XMLWriter writer(stream, true);
  writer.startElement("config");
  write(writer, flags);
  writer.endElement("config");
}


void Application::evalShutdown(const Context &ctx) {
  requestExit();
}


void Application::evalUptime(const Context &ctx) {
  ctx.stream << HumanTime((uint64_t)getUptime());
}


void Application::evalGetInfo(const Context &ctx) {
  ctx.stream << Info::instance().get(ctx.args[1], ctx.args[2]);
}


void Application::evalInfo(const Context &ctx) {
  ctx.stream << PyON::Message("info", Info::instance().getJSONList());
}


void Application::evalOption(const Script::Context &ctx) {
  string name = ctx.args[1];

  if (options.has(name)) {
    if (ctx.args.size() > 2) options[name].set(ctx.args[2]);
    else if (options[name].hasValue()) ctx.stream << options[name];

  } else THROWS("Invalid option '" << name << "'");
}


void Application::evalOptions(const Context &ctx) {
  evalOptions(ctx, options);
}


void Application::evalOptions(const Context &ctx, Options &options) {
  bool defaults = false;
  bool all = false;
  vector<string> names;
  cb::SmartPointer<JSON::Dict> dict = new JSON::Dict;

  // Process args
  for (unsigned i = 1; i < ctx.args.size(); i++) {
    if (ctx.args[i] == "-d") defaults = true;
    else if (ctx.args[i] == "-a") all = true;
    else names.push_back(ctx.args[i]);
  }

  std::set<string> added;

  if (names.empty()) names.push_back("*");

  for (unsigned i = 0; i < names.size(); i++) {
    string name = names[i];

    if (name == "*") {
      Options::const_iterator it;
      for (it = options.begin(); it != options.end(); it++) {
        Option &option = *it->second;

        if (!all && !option.isSet() && (!defaults || !option.hasValue()))
          continue;

        // Avoid duplicates
        if (added.find(option.getName()) != added.end()) continue;
        added.insert(option.getName());

        // Add to dictionary
        if (option.hasValue())
          dict->insert(option.getName(), option.toString());
        else dict->insertNull(option.getName());
      }

    } else {
      size_t equal = name.find('=');

      // Set
      if (equal != string::npos) {
        string value = name.substr(equal + 1);
        name = name.substr(0, equal);

        options.localize(name)->set(value);
        // TODO validate value for Option type
      }

      // Reset
      if (name.length() && name[name.length() - 1] == '!') {
        name = name.substr(0, name.length() - 1);
        if (options.local(name)) options[name].reset();
      }

      added.insert(name);
      if (options.has(name)) {
        // Add to dictionary
        Option &option = options[name];
        if (option.hasValue())
          dict->insert(option.getName(), option.toString());
        else dict->insertNull(option.getName());
      }
      // TODO else report error
    }
  }

  // Output
  if (!dict->empty()) ctx.stream << PyON::Message(ctx.args[0], dict);
}


void Application::evalSave(const Script::Context &ctx) {
  if (ctx.args.size() == 2) saveConfig(ctx.args[1]);
  else saveConfig();
}


int Application::printAction() {
  print(*LOG_RAW_STREAM());
  exit(0);
  return -1;
}


int Application::infoAction() {
  Info::instance().print(*LOG_RAW_STREAM());
  exit(0);
  return -1;
}


int Application::versionAction() {
  LOG_RAW(version);
  exit(0);
  return -1;
}


int Application::configAction(Option &option) {
  openConfig(option.toString());
  return 0;
}


void Application::handleSignal(int sig) {
  if (hasFeature(FEATURE_PROCESS_CONTROL) &&
      options["service"].toBoolean() && sig == SIGHUP) {
    LOG_INFO(1, "Service ignoring hangup/logoff signal");
    return;
  }

  requestExit();

  ExitSignalHandler::handleSignal(sig);
}
