#include <iostream>

#include <cbang/String.h>
#include <cbang/ServerApplication.h>
#include <cbang/ApplicationMain.h>

#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>

#include <cbang/script/Server.h>
#include <cbang/script/Environment.h>
#include <cbang/script/Functor.h>

using namespace std;
using namespace cb;
using namespace cb::Script;


void hello(const Context &ctx) {
  ctx.stream << "Hello World!\n";
}


void print(const Context &ctx) {
  Arguments::const_iterator it = ctx.args.begin();
  if (it != ctx.args.end()) it++; // Skip first

  while (it != ctx.args.end())
    ctx.stream << "'" << *it++ << "' ";

  ctx.stream << '\n';
}


class App : public ServerApplication {
  Server server;

public:
  App() : ServerApplication("Hello World Server"), server(getName()) {

    for (int i = 0; i < 10; i++)
      server.addListenPort(String::printf("127.0.0.1:999%d", i));

    server.set("prompt", "$(date '%Y/%m/%d %H:%M:%S')$ ");
    server.set("greeting", "$(clear)Welcome to the Hello World Server\n\n");
    server.add(new Functor("hello", hello, 0, 0, "Print Hello World!"));
    server.add(new Functor("print", ::print, 0, 0, "Print args"));
  }

  void run() {
    server.start();
    while (!quit) Timer::sleep(0.1);
    server.join();
  }
};


int main(int argc, char *argv[]) {
  return cb::doApplication<App>(argc, argv);
}
