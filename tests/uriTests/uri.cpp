#include <cbang/Exception.h>
#include <cbang/net/URI.h>

#include <exception>
#include <iostream>
#include <iomanip>

using namespace cb;
using namespace std;

int main(int argc, char *argv[]) {
  try {
    unsigned int w = 10;

    for (int i = 1; i < argc; i++) {
      cout << "" << argv[i] << " => ";

      try {
        URI uri(argv[i]);

        cout << "{" << endl;
        cout << setw(w) << "URI: " << uri << endl;
        cout << setw(w) << "Scheme: " << uri.getScheme() << endl;
        cout << setw(w) << "Host: " << uri.getHost() << endl;
        cout << setw(w) << "Port: " << uri.getPort() << endl;
        cout << setw(w) << "Path: " << uri.getPath() << endl;
        cout << setw(w) << "User: " << uri.getUser() << endl;
        cout << setw(w) << "Pass: " << uri.getPass() << endl;
        cout << setw(w) << "Query: ";
        for (URI::iterator it = uri.begin(); it != uri.end(); it++) {
          if (it != uri.begin()) cout << ", ";
          cout << it->first << "='" << it->second << "'";
        }
        cout << endl;

        cout << "}";

      } catch (const Exception &e) {
        cout << "INVALID: " << e.getMessage() << endl;
      }

      cout << endl;
    }

    return 0;

  } catch (const Exception &e) {
    cerr << "Exception: " << e << endl;

  } catch (const std::exception &e) {
    cerr << "std::exception: " << e.what() << endl;
  }

  return 1;
}

