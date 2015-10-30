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

#include "Transaction.h"
#include "App.h"

#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>

using namespace std;
using namespace cb;
using namespace bbmc;


Transaction::Transaction(App &app, evhttp_request *req) :
  Request(req), app(app) {
  LOG_DEBUG(5, "Transaction()");
}


Transaction::~Transaction() {
  LOG_DEBUG(5, "~Transaction()");
}


bool Transaction::apiNotFound() {
  THROWXS("Invalid API method " << getURI().getPath(), HTTP_NOT_FOUND);
  return true;
}


bool Transaction::notFound() {
  THROWXS("Not found " << getURI().getPath(), HTTP_NOT_FOUND);
  return true;
}
