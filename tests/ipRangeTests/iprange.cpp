#include <cbang/net/IPRangeSet.h>

#include <iostream>

using namespace cb;
using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 3) {
    cerr << "Usage: " << argv[0] << " <IP ranges> [test IP]" << endl;
    return 1;
  }

  IPRangeSet set(argv[1]);
  cout << set << endl;

  if (argc == 3) {
    bool contains = set.contains(IPAddress(argv[2]));
    cout << (contains ? "true" : "false") << endl;
    return contains ? 0 : 1;
  }

  return 0;
}
