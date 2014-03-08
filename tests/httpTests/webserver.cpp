#include <iostream>

#include <cbang/String.h>
#include <cbang/ServerApplication.h>
#include <cbang/ApplicationMain.h>

#include <cbang/http/Server.h>
#include <cbang/http/Handler.h>
#include <cbang/http/Context.h>
#include <cbang/http/Connection.h>

#include <cbang/time/Timer.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


class App : public ServerApplication, public Handler {
  Server server;

public:
  App() :
    ServerApplication("Web Server"), cb::HTTP::Handler(".*"), server(options) {

    // Add handler
    server.addHandler(this);
    server.addListenPort(IPAddress("0.0.0.0:8080"));
  }

  // From ServerApplication
  void run() {
    server.start();
    while (!quit) Timer::sleep(0.1);
    server.join();
  }

  // From Handler
  void buildResponse(Context *ctx) {
    Connection &con = ctx->getConnection();
    Request &request = con.getRequest();
    const URI &uri = request.getURI();
    Response &response = con.getResponse();

    response.setContentTypeFromExtension(uri.getPath());

    if (uri.getPath() == "/hello.html")
      con << "<http><body><b>Hello World!</b></body></html>" << flush;
    else if (uri.getPath() != "/empty.html")
      con << "<http><body><b>OK</b></body></html>" << flush;
  }
};


int main(int argc, char *argv[]) {
  return cb::doApplication<App>(argc, argv);
}
