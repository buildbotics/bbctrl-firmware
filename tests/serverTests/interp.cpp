#include <iostream>

#include <cbang/script/Processor.h>
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


int main(int argc, char *argv[]) {
  Processor processor;
  Environment env("Main");

  env.add(new Functor("hello", hello, 0, 0, "Print Hello World!"));
  env.add(new Functor("print", print, 0, ~0, "Print args"));

  processor.run(env, cin, cout);

  return 0;
}
