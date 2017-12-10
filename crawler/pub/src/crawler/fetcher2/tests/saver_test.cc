#include "base/common/basic_types.h"
#include "base/testing/gtest.h"
#include "base/common/logging.h"

#include "crawler/fetcher2/saver.h"

namespace crawler {

// TEST(TestSaver, StoreProtoBuffer) {
//   CrawledResourceSaver saver("./test_saver", "proto", 100);
//   CrawledResource res;
//   res.set_source_url("http://www.sohu.com/");
//   res.set_effective_url("http://www.sohu.com/");
//   res.set_content_raw("abcd");
//   res.set_http_header_raw("header");
//   res.set_resource_type(kHTML);
//   CHECK(saver.StoreProtoBuffer(res));
// }

}  // namespace crawler
