#include <vector>
#include <string>
#include <map>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/strings/string_util.h"
#include "base/encoding/base64.h"
#include "base/encoding/cescape.h"
#include "base/encoding/url_encode.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "web/url/url.h"
#include "web/url/url_util.h"
#include "base/time/time.h"

#include "log_analysis/common/log_common.h"

namespace log_analysis {

typedef bool (*BaiduUrlCrackMethod)(const std::string&, std::string*);

static int ParseHex(char c) {
  if ('A' <= c && c <= 'F')
    return c - 'A' + 10;
  else if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  else
    return c - '0';
}

static bool method1(const std::string& cypher, std::string* url) {
  const uint64 key_len = 598;
  const uint64 key_idx = 76;
  const char* key = "8b4ca5e9e985fb21525520cf9801a5cef5189968e11f8aac6722de2f368501eed8d93a9e8263682435f15105a7af9ec3e095471498b08cb375113e642c40a7ec52df1c3c2fc4c2a11ca52d310c763faab58868b897af221ea35cd247dff983fb517d7427b4698a36828117c33152b43758c17cc6d356eef99cc6a3cd24cc2d13c88c0f1a38191996599620f95bf79e684d66a873dab091d0ac52cd2d39e4864d38fac200d06894059cf8b75d654cb02f956393a37e32ea5c571b3c63c489d5dcf9f22ea2541c37f98a98e3119e0eeca866013be790329f08fa7baeffbf171192f4564d29f5bf7bc3e0ea1924d5dab4ea859159ee76d748b6014cbaf7629e9c96ca78840e9440276c02d4dd1daae436f6faaa3826a2d4957b7de35afda08dc7cccee47dfbc768d957020015";  // NOLINT
  if (cypher.size() <= key_idx ||
      cypher.substr(62, 14) != "a6e8c0962218c9") {
    *url = "";
    return false;
  }

  std::string encrypted = cypher.substr(key_idx);

  if (encrypted.size() > key_len || encrypted.size() % 2u == 1u) {
    *url = "";
    return false;
  }
  url->clear();
  url->reserve(2*key_len);
  for (int i = 0; i < (int)encrypted.size(); i+=2) {
    uint32 hi_cypher = ParseHex(encrypted[i]);
    uint32 lo_cypher = ParseHex(encrypted[i+1]);
    uint32 hi_key = ParseHex(key[i]);
    uint32 lo_key = ParseHex(key[i+1]);
    uint32 hi = hi_cypher ^ hi_key;
    uint32 lo = lo_cypher ^ lo_key;

    uint32 num = (hi << 4) + lo;
    if (num > 127u || num <= 32u) {
      url->append(base::StringPrintf("%%%02x", num));
    } else {
      url->append(1, (char)(uint8)num);
    }
  }
  return true;
}
static bool method2(const std::string& cypher, std::string* url) {
  const size_t key_len = 728;
  const size_t key_idx = 744;
  const char* key = "a811e62785e07b1350a4ab8e9e155cd270630b2cb708db2e53459603ce508408e3c76cde66d4710fcc9273b4bf1d90e4766417532a313ccfedbb5ab4badc03f9d808fc8ae234abe51d1648bdc9fce2049cb7516b7fa7dc2ddc174f40293852c965eb467c846634599efcddc56950f194e4c6bd65b52d2c855a5e75da9203ce1f74fc4088e12131ecd98067b6f44007ed4e12da4309f37e455e61ff48efe31eadc50e1916b5d89f73500db19a87c27110673795fe6ed18f1620b1fc7c20ce45e662460c5f698a39069ebff0e97686a98b3687ad7d6506a9c974a7a4549ba8e21e52a1582e49e88f84f0c92eeff8058e94e7b3724c3193055fe66ac43d5f9c74edc3a0b5b31e884c4c1a966438fa10e4fec7cdf0f073f272d1c6c43173dc4acc1850e690ef2eb71eaf92db5ee6027581512c4eff2adfc4f9942350e2b63fd375d2a3bf7ef0f28ef50f9b215d34147af1";  // NOLINT
  if (cypher.size() <= key_idx ||
      cypher.substr(730, 14) != "ebac5573358cc3") {
    *url = "";
    return false;
  }

  std::string encrypted = cypher.substr(key_idx);
  if (encrypted.size() > key_len || encrypted.size() % 2u == 1u) {
    *url = "";
    return false;
  }
  url->clear();
  url->reserve(2*key_len);
  for (int i = 0; i < (int)encrypted.size(); i+=2) {
    uint32 hi_cypher = ParseHex(encrypted[i]);
    uint32 lo_cypher = ParseHex(encrypted[i+1]);
    uint32 hi_key = ParseHex(key[i]);
    uint32 lo_key = ParseHex(key[i+1]);
    uint32 hi = hi_cypher ^ hi_key;
    uint32 lo = lo_cypher ^ lo_key;

    uint32 num = (hi << 4) + lo;
    if (num > 127u || num <= 32u) {
      url->append(base::StringPrintf("%%%02x", num));
    } else {
      url->append(1, (char)(uint8)num);
    }
  }
  return true;
}

static const BaiduUrlCrackMethod BaiduUrlCrackers[] = {
  method1,
  method2,
};

bool ParseBaiduTargetUrl(const std::string& orig_url, std::string* target_url) {
  *target_url = "";
  const std::string& url = orig_url;

  GURL gurl(url);
  if (!gurl.is_valid() || !gurl.has_host()) {
    DLOG(INFO) << "host/ incomplete in url: " << url;
    return false;
  }
  if (!base::StartsWith(url, "http://www.baidu.com/link?url=", false)) {
    *target_url = url;
    return true;
  } else {
    std::string cracked;
    bool final_success = false;
    for (int32 i = 0; i < (int32)ARRAYSIZE_UNSAFE(BaiduUrlCrackers); ++i) {
      bool succ = BaiduUrlCrackers[i](url, &cracked);
      DLOG(ERROR) << "succ: " << succ;
      DLOG(ERROR) << "cracked: " << cracked;
      if (succ &&
          (base::StartsWith(cracked, "http", false) ||
           base::StartsWith(cracked, "ftp", false))) {
        final_success = true;
        *target_url = cracked;
        break;
      }
    }
    if (final_success) {
      GURL gurl(*target_url);
      if (!gurl.is_valid() || !gurl.has_host()) {
        final_success = false;
      } else if (target_url->find_first_of("\t\r\n") != std::string::npos) {
        final_success = false;
      } else {
        std::string result;
        if (!base::DecodeUrlComponent(target_url->c_str(), &result)) {
          final_success = false;
        }
      }
    }
    return final_success;
  }
}
}  // namespace
