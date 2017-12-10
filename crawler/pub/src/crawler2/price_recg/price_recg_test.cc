#include "crawler2/price_recg/price_recg.h"

#include <fstream>
#include <string>

#include "base/testing/gtest.h"
#include "base/common/base.h"
#include "base/common/logging.h"

namespace crawler2 {

TEST(PriceOCR, Train) {
  AsciiRecognize recg;
  std::string path = "./crawler2/price_recg/data/";
  ASSERT_TRUE(recg.Init(path + "/training", path + "/testing"));
  std::string result;
  // Image on disk
  ASSERT_TRUE(recg.PriceDiskImage("crawler2/price_recg/data/testing/$15599.00.png", &result));
  ASSERT_STREQ(result.c_str(), "$15599.00");

  // Image on memory
  std::ifstream is("crawler2/price_recg/data/testing/$15599.00.png", std::ios::binary);
  CHECK(is.good());
  is.seekg(0, std::ifstream::end);
  uint64 size = is.tellg();
  is.seekg(0);
  std::string pic;
  pic.resize(size);
  is.read(&pic[0], size);
  result.clear();
  ASSERT_TRUE(recg.PriceVMemImage(pic, "png", &result));
  ASSERT_STREQ(result.c_str(), "$15599.00");
}
}

