#include <iostream>

#include <cbang/String.h>
#include <cbang/ServerApplication.h>
#include <cbang/ApplicationMain.h>

#include <cbang/http/Server.h>
#include <cbang/http/Handler.h>
#include <cbang/http/Context.h>
#include <cbang/http/Connection.h>

#include <cbang/os/SystemUtilities.h>

#include <cbang/openssl/SSLContext.h>
#include <cbang/openssl/BIMemory.h>

#include <cbang/time/Timer.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


class App : public ServerApplication, public Handler {
  Server server;
  SSLContext sslCtx;

public:
  App() :
    ServerApplication("Web Server"), cb::HTTP::Handler(".*"), server(options) {

    // Add handler
    server.addHandler(this);
    server.addListenPort(IPAddress("0.0.0.0:8084"), &sslCtx);
  }

  // From ServerApplication
  void run() {
    // Require a valid certificate from client
    sslCtx.setVerifyPeer(true, true);

    // Load certificates and keys
    sslCtx.useCertificateChainFile("server.pem");
    sslCtx.usePrivateKey(InputSource("server.pem"));
    //sslCtx.addTrustedCA("root.pem");
    //sslCtx.addCRL("crl.pem");

    // Test BIMemory
    uint64_t length = SystemUtilities::getFileSize("root.pem");
    SmartPointer<iostream> f = SystemUtilities::open("root.pem", ios::in);
    SmartPointer<char> buf = new char[length];
    f->read(buf.get(), length);
    if (f->fail()) THROWS("Failed reading root.pem");

    BIMemory bio(buf.get(), length);
    sslCtx.addTrustedCA(bio.getBIO());

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
