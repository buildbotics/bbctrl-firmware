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

#include "Server.h"

#include "Connection.h"
#include "Context.h"
#include "Handler.h"

#include <cbang/Exception.h>

#include <cbang/config/Options.h>

#include <cbang/socket/SocketSet.h>
#include <cbang/socket/SocketDebugger.h>

#include <cbang/os/SystemUtilities.h>

#include <cbang/log/Logger.h>
#include <cbang/time/Time.h>
#include <cbang/time/Timer.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/security/SSLContext.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Server::Server(Options &options, SSLContext *sslCtx)
  : options(options), sslCtx(sslCtx), initialized(false),
    queueConnections(false), maxRequestLength(1024 * 1024 * 50),
    maxConnections(800), connectionTimeout(60), maxConnectTime(60 * 15),
    minConnectTime(5 * 60), captureRequests(false), captureResponses(false),
    captureOnError(false) {
  SmartPointer<Option> opt;

  options.pushCategory("HTTP Server");

  opt = options.add("http-addresses", "A space separated list of server "
                    "address and port pairs to listen on in the form "
                    "<ip | hostname>[:<port>]");
  opt->setType(Option::STRINGS_TYPE);
  opt->setDefault("0.0.0.0:80");

  if (sslCtx) {
    opt = options.add("https-addresses", "A space separated list of secure "
                      "server address and port pairs to listen on in the form "
                      "<ip | hostname>[:<port>]");
    opt->setType(Option::STRINGS_TYPE);
    opt->setDefault("0.0.0.0:443");
  }

  options.addTarget("max-connections", maxConnections,
                    "Sets the maximum number of simultaneous connections.");
  options.addTarget("max-request-length", maxRequestLength,
                    "Sets the maximum length of a client request packet.");
  options.addTarget("connection-timeout", connectionTimeout, "The maximum "
                    "amount of time, in seconds, a connection can be idle "
                    "before being dropped.");
  options.addTarget("max-connect-time", maxConnectTime, "The maximum amount of "
                    "time, in seconds, a client can be connected to the "
                    "server.");
  options.addTarget("min-connect-time", minConnectTime, "The minimum amount "
                    "of time, in seconds, a client must be connected to the "
                    "server before it can be dropped in favor or a new "
                    "connection.");

  options.add("allow", "Client addresses which are allowed to connect to this "
              "server.  This option overrides IPs which are denied in the "
              "deny option.  The pattern 0/0 matches all addresses."
              )->setDefault("0/0");
  options.add("deny", "Client address which are not allowed to connect to this "
              "server.")->setType(Option::STRINGS_TYPE);

  options.popCategory();

  if (sslCtx) {
    options.pushCategory("HTTP Server SSL");
    options.add("crl-file", "Supply a Certificate Revocation List.  Overrides "
                "any internal CRL");
    options.add("certificate-file", "The servers certificate file in PEM "
                "format.")->setDefault("certificate.pem");
    options.add("private-key-file", "The servers private key file in PEM "
                "format.")->setDefault("private.pem");
    options.popCategory();
  }

  // TODO should probably remove these in favor of the new Socket level capture
  options.pushCategory("Debugging");
  options.add("capture-packets", "Capture HTTP request and response packets "
              "and save them to 'capture-directory'."
              )->setDefault(false);
  options.addTarget("capture-requests", captureRequests, "Capture HTTP "
                    "requests and save them to 'capture-directory'.");
  options.addTarget("capture-responses", captureResponses, "Capture HTTP "
                    "responses and save them to 'capture-directory'.");
  options.addTarget("capture-on-error", captureOnError, "Capture HTTP request "
                    "and response packets only in case of errors.");
  options.popCategory();
}


Handler *Server::match(Connection *con) {
  for (unsigned i = 0; i < handlers.size(); i++)
    if (handlers[i]->match(con)) return handlers[i];

  return 0;
}


void Server::captureRequest(Connection *con) {
  SystemUtilities::ensureDirectory(captureDir);

  try {
    string filename = getCaptureFilename("request", con->getID());
    con->writeRequest(*SystemUtilities::open(filename, ios::out | ios::trunc));

  } catch (const Exception &e) {
    LOG_WARNING("Packet capture request " << con->getID() << " failed: " << e);
  }
}


void Server::captureResponse(Connection *con) {
  SystemUtilities::ensureDirectory(captureDir);

  try {
    string filename = getCaptureFilename("response", con->getID());
    con->writeResponse(*SystemUtilities::open(filename, ios::out | ios::trunc));

  } catch (const Exception &e) {
    LOG_WARNING("Packet capture response " << con->getID() << " failed: " << e);
  }
}


void Server::init() {
  if (initialized) return;
  initialized = true;

  // IP filters
  ipFilter.allow(options["allow"]);
  ipFilter.deny(options["deny"]);

  // Configure ports
  Option::strings_t addresses = options["http-addresses"].toStrings();
  for (unsigned i = 0; i < addresses.size(); i++)
    addListenPort(addresses[i]);

  // SSL
  if (sslCtx) {
    // Configure secure ports
    addresses = options["https-addresses"].toStrings();
    for (unsigned i = 0; i < addresses.size(); i++)
      addListenPort(addresses[i], sslCtx);

    // Load server certificate
    // TODO should load file relative to configuration file
    if (options["certificate-file"].hasValue()) {
      string certFile = options["certificate-file"].toString();
      if (SystemUtilities::exists(certFile))
        sslCtx->useCertificateChainFile(certFile);
      else LOG_WARNING("Certificate file not found " << certFile);
    }

    // Load server private key
    if (options["private-key-file"].hasValue()) {
      string priKeyFile = options["private-key-file"].toString();
      if (SystemUtilities::exists(priKeyFile))
        sslCtx->usePrivateKey(priKeyFile);
      else LOG_WARNING("Private key file not found " << priKeyFile);
    }

    // CRL
    if (options["crl-file"].hasValue()) {
      string crlFile = options["crl-file"];
      if (SystemUtilities::exists(crlFile)) sslCtx->addCRL(crlFile);
      else LOG_WARNING("Certificate Relocation List not found " << crlFile);
    }
  }

  // Packet capture
  if (options["capture-packets"].toBoolean())
    captureRequests = captureResponses = true;
  if (captureOnError) captureRequests = captureResponses = false;
  captureDir = SocketDebugger::instance().getCaptureDirectory();
}


bool Server::handleConnection(double timeout) {
  try {
    SocketConnectionPtr ptr = queue.next(timeout); // Hold this pointer
    Connection *con = ptr.castPtr<Connection>();

    if (!con) return false;
    if (con->getState() == Connection::PROCESSING) process(*con);

  } CATCH_ERROR;

  return true;
}


void Server::createThreadPool(unsigned size) {
  stopThreadPool();
  pool.clear();

  // Process connections via connection queue
  queueConnections = true;

  for (unsigned i = 0; i < size; i++)
    pool.push_back(new ThreadFunc<Server>(this, &Server::poolThread));
}


void Server::startThreadPool() {
  for (pool_t::iterator it = pool.begin(); it != pool.end(); it++)
    (*it)->start();
}


void Server::stopThreadPool() {
  for (pool_t::iterator it = pool.begin(); it != pool.end(); it++)
    (*it)->stop();
}


void Server::joinThreadPool() {
  for (pool_t::iterator it = pool.begin(); it != pool.end(); it++)
    (*it)->join();
}


void Server::start() {
  if (getState() != THREAD_STOPPED) THROW("HTTPServer already running");
  startThreadPool();
  SocketServer::start();
}


void Server::stop() {
  Thread::stop();
  queue.shutdown();
  stopThreadPool();
}


void Server::join() {
  joinThreadPool();
  Thread::join();
}


void Server::process(Connection &con) {
  con.delayUntil(0);
  con.process();
  //con.flush();  // Calling flush() here triggers a bug in boost::iostreams

  // If the processing code requested that we delay then go to the
  // DELAY_PROCESSING state otherwise proceed directly to WRITING_HEADER.
  con.setState(con.getRestartTime() ? Connection::DELAY_PROCESSING :
               Connection::WRITING_HEADER);
}


void Server::poolThread() {
  while (!shouldShutdown()) handleConnection(0.1);
}


void Server::processConnection(const SocketConnectionPtr &_con, bool ready) {
  Connection *con = _con.castPtr<Connection>();

  // NOTE: Don't access Context from cb::HTTP while PROCESSING
  if (ready && con->getState() != Connection::PROCESSING &&
      con->getContext() && !con->getContext()->isReady()) ready = false;

  do {
    try {
      switch (con->getState()) {
      case Connection::READING_HEADER: {
        if (!ready || !con->readHeader()) break;

        if (con->getResponse().getStatus()) {
          // Already have a response
          con->setState(Connection::WRITING_HEADER);
          continue;
        }

        Handler *handler = match(con);

        if (handler) {
          // TODO ipFilter should probably be moved to a HTTP::SecurityHandler
          if (!handler->allow(con) || !ipFilter.isAllowed(con->getClientIP())) {
            con->fail(StatusCode::HTTP_FORBIDDEN);
            con->setState(Connection::WRITING_HEADER);

            LOG_WARNING("Denied " << *con << " access to URI: "
                        << con->getRequest().getURI());
            continue;

          } else {
            con->setHandler(handler);
            con->setContext(handler->createContext(con));

            con->setState(Connection::READING_DATA);
            // Fall through
          }

        } else {
          con->fail(StatusCode::HTTP_NOT_FOUND);
          con->setState(Connection::WRITING_HEADER);
          continue;
        }
      }

      case Connection::READING_DATA:
        if (!ready || !con->read()) break;

        if (captureRequests) captureRequest(con);
        con->setState(Connection::PROCESSING);

        // Add the connection SmartPointer to the queue
        if (queueConnections) {
          queue.add(_con);
          break;
        }

      case Connection::PROCESSING:
        // Process the connection directly in this thread if connections are
        // not being queued for processing externally.  This allows the HTTP
        // server to run with out threads.
        if (!queueConnections) process(*con);
        break;

      case Connection::DELAY_PROCESSING:
        if (con->getRestartTime() < Timer::now()) {
          con->setState(Connection::PROCESSING);

          // Put connection SmartPointer back on the queue
          if (queueConnections) queue.add(_con);
        }
        break;

      case Connection::WRITING_HEADER:
        if (!ready || !con->writeHeader()) break;
        if (captureResponses) captureResponse(con);
        con->setState(Connection::WRITING_DATA);

      case Connection::WRITING_DATA:
        if (!ready || !con->write()) break;
        con->setState(Connection::REPORTING);

        if (con->isPersistent()) {
          con->reportResults();
          con->setState(Connection::READING_HEADER);
          continue;
        }

      case Connection::REPORTING:
        con->reportResults();
        con->setState(Connection::CLOSING);

      case Connection::CLOSING: break;

      default:
        LOG_ERROR("Processing connection " << con->getID()
                  << ", invalid state:" << con->getState());
      }

      // Timeouts
      if (con->getState() < Connection::REPORTING &&
          (connectionTimeout < Time::now() - con->getLastIO() ||
           maxConnectTime < Time::now() - con->getStartTime())) {
        con->setState(Connection::REPORTING);
        LOG_DEBUG(3, *con << ": Connection timed out");
        continue;
      }

      return; // Done processing this connection

    } catch (const Exception &e) {
      if (LOG_DEBUG_ENABLED(3)) LOG_DEBUG(3, *con << ": " << e);
      else LOG_INFO(3, *con << ": " << e.getMessage());

    } catch (const std::exception &e) {
      LOG_ERROR("std::exception: " << *con << ": " << e.what());
    }

    if (con->getState() < Connection::REPORTING) {
      if (captureOnError) captureRequest(con);
      con->fail();
      con->setState(Connection::REPORTING);

    } else return;

  } while (true);
}


void Server::limitConnections() {
  // Check max connections and elect to drop an old connection if full
  if (maxConnections <= connections.size()) {
    vector<Connection *> oldest;
    unsigned count = connections.size() - maxConnections + 1;

    // Find oldest connections
    connections_t::iterator it;
    for (it = connections.begin(); it != connections.end(); it++) {
      Connection *con = it->castPtr<Connection>();

      if (Time::now() - con->getStartTime() < minConnectTime) continue;
      if (con->getState() == Connection::PROCESSING) continue;

      if (oldest.size() < count) oldest.push_back(con);
      else {
        // Found an older connection, find youngest of the oldest and replace
        unsigned youngest = 0;
        for (unsigned i = 1; i < oldest.size(); i++)
          if (oldest[youngest]->getStartTime() < oldest[i]->getStartTime())
            youngest = i;

        if (con->getStartTime() < oldest[youngest]->getStartTime())
          oldest[youngest] = con;
      }
    }

    // Drop oldest connections
    for (unsigned i = 0; i < oldest.size(); i++) {
      LOG_WARNING("Max connections exceeded dropping connection "
                  << oldest[i]->getID());
      oldest[i]->setState(Connection::CLOSING);
    }
  }

  // Actually removes connections from the server
  cleanupConnections();
}


SocketConnectionPtr
Server::createConnection(SmartPointer<Socket> socket,
                         const IPAddress &clientIP) {
  limitConnections();
  if (maxConnections <= connections.size()) return 0;
  return new Connection(*this, socket, clientIP);
}


void Server::addConnectionSockets(SocketSet &sockSet) {
  SocketServer::addConnectionSockets(sockSet);

  for (iterator it = begin(); it != end(); it++) {
    Connection &con = *it->castPtr<Connection>();

    switch (con.getState()) {
    case Connection::READING_HEADER:
    case Connection::READING_DATA:
      sockSet.add(con.getSocket(), SocketSet::READ);
      break;

    case Connection::WRITING_HEADER:
    case Connection::WRITING_DATA:
      // TODO check if streamed data has more and warn
      sockSet.add(con.getSocket(), SocketSet::WRITE);
      break;

    default: break;
    }
  }
}


bool Server::connectionsReady() const {
  for (iterator it = begin(); it != end() && !shouldShutdown(); it++) {
    Connection::state_t state = it->castPtr<Connection>()->getState();
    if (state == Connection::DELAY_PROCESSING) return true;
    if (!queueConnections && state == Connection::PROCESSING) return true;
  }

  return false;
}


void Server::processConnections(SocketSet &sockSet) {
  if (!initialized) THROW("HTTP::Server not initialized");

  // Save current thread ID and restore it before leaving this scope
  struct SmartThreadID {
    unsigned threadID;
    SmartThreadID() : threadID(Logger::instance().getThreadID()) {}
    ~SmartThreadID() {Logger::instance().setThreadID(threadID);}
  } smartThreadID;

  for (iterator it = begin(); it != end() && !shouldShutdown(); it++) {
    // Set thread ID = connection ID
    Logger::instance().setThreadID((*it)->getID());
    processConnection(*it, sockSet.isSet((*it)->getSocket()));
  }
}


void Server::closeConnection(const SocketConnectionPtr &con) {
  con.castPtr<Connection>()->setState(Connection::CLOSING);
}


string Server::getCaptureFilename(const string &name, unsigned id) {
  return captureDir + "/" + name + "." +
    Time("%Y%m%d-%H%M%S.").toString() + String(id);
}
