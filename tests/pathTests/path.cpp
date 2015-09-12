#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/util/DefaultCatch.h>

#include <iostream>
#include <iomanip>

using namespace cb;
using namespace std;


void usage(const char *name) {
  cout
    << "Usage: " << name << " [OPTIONS]\n\n"
    << "OPTIONS:\n"
    << "\t--help                           Print this help screen and exit.\n"
    << endl;
}


int main(int argc, char *argv[]) {
  try {
    for (int i = 1; i < argc; i++) {
      string arg = argv[i];

      if (arg == "--help") {
        usage(argv[0]);
        return 0;

      } else if (arg == "--basename" && i < argc - 1) {
        cout << "'" << SystemUtilities::basename(argv[++i]) << "'" << endl;

      } else if (arg == "--dirname" && i < argc - 1) {
        cout << "'" << SystemUtilities::dirname(argv[++i]) << "'" << endl;

      } else if (arg == "--extension" && i < argc - 1) {
        cout << "'" << SystemUtilities::extension(argv[++i]) << "'" << endl;

      } else if (arg == "--is_absolute" && i < argc - 1) {
        cout << String(SystemUtilities::isAbsolute(argv[++i])) << endl;

      } else if (arg == "--canonical" && i < argc - 1) {
        cout << String(SystemUtilities::getCanonicalPath(argv[++i])) << endl;

      } else {
        usage(argv[0]);
        THROWS("Invalid arg '" << arg << "'");
      }
    }

    return 0;

  } CATCH_ERROR;

  return 1;
}

