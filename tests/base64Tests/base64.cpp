#include <cbang/net/Base64.h>

#include <cbang/util/DefaultCatch.h>

#include <iostream>

using namespace std;
using namespace cb;


int usage(const char *name) {
  cerr << "Usage: " << name << " <-d | -e> <string>" << endl;
  return 1;
}


int main(int argc, char *argv[]) {
  try {
    if (argc != 3) return usage(argv[0]);

    if (string("-d") == argv[1]) cout << Base64().decode(argv[2]) << endl;
    else if (string("-e") == argv[1]) cout << Base64().encode(argv[2]) << endl;
    else return usage(argv[0]);

    return 0;

  } CBANG_CATCH_ERROR;
  return 1;
}
