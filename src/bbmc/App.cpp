/******************************************************************************\

             This file is part of the Buildbotics Machine Controller.

                    Copyright (c) 2015-2016, Buildbotics LLC
                               All rights reserved.

        The Buildbotics Machine Controller is free software: you can
        redistribute it and/or modify it under the terms of the GNU
        General Public License as published by the Free Software
        Foundation, either version 2 of the License, or (at your option)
        any later version.

        The Buildbotics Webserver is distributed in the hope that it will
        be useful, but WITHOUT ANY WARRANTY; without even the implied
        warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
        PURPOSE.  See the GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this software.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#include "App.h"

#include <cbang/util/DefaultCatch.h>
#include <cbang/log/Logger.h>
#include <cbang/event/Event.h>

#include <stdlib.h>
#include <unistd.h>

using namespace bbmc;
using namespace cb;
using namespace std;


App::App() :
  ServerApplication("Buildbotics Machine Controller", &App::_hasFeature),
  dns(base), client(base, dns), server(*this) {

  options.pushCategory("Buildbotics Machine Controller");
  options.add("http-root", "Serve /* files from this directory.");
  options.popCategory();

  options.pushCategory("Debugging");
  options.add("debug-libevent", "Enable verbose libevent debugging"
              )->setDefault(false);
  options.popCategory();

  // Enable libevent logging
  Event::Event::enableLogging(3);
}


bool App::_hasFeature(int feature) {
  if (feature == FEATURE_LIFELINE) return true;
  return ServerApplication::_hasFeature(feature);
}

int App::init(int argc, char *argv[]) {
  int i = ServerApplication::init(argc, argv);
  if (i == -1) return -1;

  // Libevent debugging
  if (options["debug-libevent"].toBoolean()) Event::Event::enableDebugLogging();

  server.init();

  // Check lifeline
  if (getLifeline()) base.newEvent(this, &App::lifelineEvent).add(0.25);

  // Handle exit signal
  base.newSignal(SIGINT, this, &App::signalEvent).add();
  base.newSignal(SIGTERM, this, &App::signalEvent).add();

  return 0;
}


void App::run() {
  try {
    base.dispatch();
    LOG_INFO(1, "Clean exit");
  } CATCH_ERROR;
}


void App::lifelineEvent(Event::Event &e, int signal, unsigned flags) {
  e.add(0.25);
  if (shouldQuit()) base.loopExit();
}


void App::signalEvent(Event::Event &e, int signal, unsigned flags) {
  base.loopExit();
}
