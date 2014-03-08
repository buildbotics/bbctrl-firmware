#include <cbang/util/DefaultCatch.h>

#include <cbang/json/Value.h>
#include <cbang/json/Reader.h>
#include <cbang/json/Writer.h>

#include <iostream>

using namespace std;
using namespace cb::JSON;


int main(int argc, char *argv[]) {
  try {
    Reader reader(cin);
    ValuePtr data = reader.parse();
    if (!data.isNull()) cout << *data;

    return 0;

  } CBANG_CATCH_ERROR;
  return 0;
}
