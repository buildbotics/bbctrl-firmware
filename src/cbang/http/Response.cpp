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

#include "Response.h"

#include <cbang/String.h>
#include <cbang/Exception.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/net/URI.h>
#include <cbang/time/Time.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


void Response::reset() {
  status = StatusCode::HTTP_UNKNOWN;
  finalized = false;
  Message::reset();
}


void Response::redirect(const URI &uri, StatusCode status) {
  LOG_DEBUG(5, "Redirect: " << status << ": " << uri);
  setStatus(status);
  set("Location", uri);
  set("Content-Length", "0");
}


string Response::getHeaderLine() const {
  return
    getVersionString() + " " + String(status) + " " + string(status.toString());
}


void Response::readHeaderLine(const string &line) {
  vector<string> parts;
  String::tokenize(line, parts);

  if (parts.size() < 3)
    THROWS("Invalid HTTP response header line '" << line << "'");

  readVersionString(parts[0]);
  status = (StatusCode::enum_t)String::parseU32(parts[1]);
}


void Response::finalize(unsigned length) {
  if (finalized) return;

  if (status == StatusCode::HTTP_UNKNOWN) status = StatusCode::HTTP_OK;

  set("Date", Time(TIME_FORMAT));

  if (isChunked()) set("Transfer-Encoding", "chunked");
  else if (length) set("Content-Length", String(length));

  if (!has("Cache-Control")) set("Cache-Control", "no-cache");

  finalized = true;
}


void Response::setCacheExpire(unsigned secs) {
  set("Expires", Time(Time::now() + secs, TIME_FORMAT).toString());
  set("Cache-Control", "max-age");
}
