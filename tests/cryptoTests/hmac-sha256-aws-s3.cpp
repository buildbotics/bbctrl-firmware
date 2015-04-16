#include <cbang/String.h>
#include <cbang/openssl/Digest.h>
#include <cbang/util/DefaultCatch.h>

using namespace std;
using namespace cb;


int main(int argc, char *argv[]) {
  // Test data from:
  // http://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-query-string-auth.html

  try {
    string key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";
    key = Digest::signHMAC("AWS4" + key, "20130524", "sha256");
    key = Digest::signHMAC(key, "us-east-1", "sha256");
    key = Digest::signHMAC(key, "s3", "sha256");
    key = Digest::signHMAC(key, "aws4_request", "sha256");

    string data =
      "AWS4-HMAC-SHA256\n"
      "20130524T000000Z\n"
      "20130524/us-east-1/s3/aws4_request\n"
      "3bfa292879f6447bbcda7001decf97f4a54dc650c8942174ae0a9121cf58ad04";

    string sig = Digest::signHMAC(key, data, "sha256");

    cout << "sig=" << String::hexEncode(sig) << endl;

    return 0;
  } CATCH_ERROR;

  return 1;
}
