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

#ifndef CBANG_APPLICATION_H
#define CBANG_APPLICATION_H

#include "SmartPointer.h"

#include <cbang/config/Options.h>
#include <cbang/config/CommandLine.h>

#include <cbang/xml/XMLReader.h>

#include <cbang/util/Version.h>
#include <cbang/util/Features.h>

#include <cbang/os/ExitSignalHandler.h>

#include <cbang/script/Environment.h>

#include <string>

namespace cb {
  class Logger;
  class Option;
}


namespace cb {
  class EnumerationManager;
  class XMLWriter;

  class Application :
    public Features, protected ExitSignalHandler, public Script::Environment {
  public:
    enum {
      FEATURE_ENUMERATION_MANAGER,
      FEATURE_PROCESS_CONTROL,
      FEATURE_CONFIG_FILE,
      FEATURE_DEBUGGING,
      FEATURE_INFO,
      FEATURE_PRINT_INFO,
      FEATURE_SCRIPT_SERVER,
      FEATURE_LAST,
    };

  protected:
    Options options; // Must be first
    CommandLine cmdLine;
    XMLReader configReader;
    Logger &logger;
    EnumerationManager *enumMan;

    const std::string name;
    Version version;
    std::string runDirectoryRegKey;

    bool configRotate;
    uint32_t configRotateMax;
    std::string configRotateDir;

    bool initialized;
    bool configured;
    volatile mutable bool quit;

    double startTime;

  public:
    Application(const std::string &name,
                hasFeature_t hasFeature = Application::_hasFeature);
    virtual ~Application();

    static bool _hasFeature(int feature);

    Options &getOptions() {return options;}
    const Options &getOptions() const {return options;}
    CommandLine &getCommandLine() {return cmdLine;}
    Logger &getLogger() {return logger;}
    XMLReader &getXMLReader() {return configReader;}
    EnumerationManager &getEnumerationManager() {return *enumMan;}

    const std::string &getName() const {return name;}
    const Version &getVersion() const {return version;}
    void setVersion(const Version &version) {this->version = version;}

    bool isConfigured() const {return configured;}
    virtual bool shouldQuit() const {return quit;}
    virtual void requestExit() {quit = true;}

    double getUptime() const;

    virtual int init(int argc, char *argv[]);
    virtual void initialize() {}
    virtual void run() {}
    virtual void printInfo() const;
    virtual void write(XMLWriter &writer, uint32_t flags = 0) const;
    virtual std::ostream &print(std::ostream &stream) const;
    virtual void usage(std::ostream &stream, const std::string &name) const;
    virtual void openConfig(const std::string &filename = std::string());
    virtual void saveConfig(const std::string &filename = std::string()) const;

    virtual void writeConfig(std::ostream &stream, uint32_t flags = 0) const;

    virtual void evalShutdown(const Script::Context &ctx);
    virtual void evalUptime(const Script::Context &ctx);
    virtual void evalGetInfo(const Script::Context &ctx);
    virtual void evalInfo(const Script::Context &ctx);
    virtual void evalOption(const Script::Context &ctx);
    virtual void evalOptions(const Script::Context &ctx);
    virtual void evalOptions(const Script::Context &ctx, Options &options);
    virtual void evalSave(const Script::Context &ctx);

  protected:
    // Command line actions
    virtual int printAction();
    virtual int infoAction();
    virtual int versionAction();
    virtual int chdirAction(Option &option);
    virtual int configAction(Option &option);

    // From SignalHandler
    void handleSignal(int sig);
  };
}

#endif // CBANG_APPLICATION_H
