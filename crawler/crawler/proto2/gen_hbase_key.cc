#include "crawler/proto2/gen_hbase_key.h"

#include <vector>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_split.h"
#include "web/url_norm/url_norm.h"
#include "web/url/url.h"

namespace crawler2 {

const uint64 kHashSeed = 2012;
const int kShardNum = 16384;
const int kNewShardNum = 500000;

bool ReverseUrl(const std::string &url, std::string *reversed_url) {
  CHECK_NOTNULL(reversed_url);
  reversed_url->clear();

  // XXX(suhua): 如果 url 不合法, 函数行为未定义, 会尽量检查出来, 返回 false

  // 找到 url 的 host
  base::Slice host = url;

  // 去掉协议头
  base::Slice::size_type n = host.find("://");
  if (n == base::Slice::npos) return false;
  host.remove_prefix(n + 3);

  base::Slice protocol(url.data(), n);
  if (protocol.empty()) return false;

  // 检查协议字段
  for (auto it = protocol.begin(); it != protocol.end(); ++it) {
    // 只允许数字和字母
    if ((*it >= 'a' && *it <= 'z')
        || (*it >= 'A' && *it <= 'Z')
        || (*it >= '0' && *it <= '9')) {
    } else {
      return false;
    }
  }

  // 取出路径串, 注意兼容 空 路径
  n = host.find('/');
  if (n != base::Slice::npos) {
    host.remove_suffix(host.size() - n);
  }
  base::Slice path(url.data(), url.size());
  path.remove_prefix(host.data() + host.size() - url.data());

  // 找到用户名和密码
  n = host.find('@');
  base::Slice user_pass;
  if (n != base::Slice::npos) {
    user_pass.set(host.data(), n);
    host.remove_prefix(n + 1);
  } else {
    user_pass.set(host.data(), 0);
  }

  // 去掉端口
  n = host.rfind(':');
  base::Slice port;
  if (n != base::Slice::npos) {
    port = host;
    port.remove_prefix(n + 1);
    host.remove_suffix(host.size() - n);
  } else {
    port.set(host.data(), 0);
  }

  if (host.empty()) return false;

  if (host.find_first_not_of("0123456789.") == base::Slice::npos) {
    // host 只有数字和 . , 不反转, 原样输出
    *reversed_url = url;
    return true;
  }

  std::vector<std::string> tokens;
  base::SplitString(host.as_string(), ".", &tokens);
  std::reverse(tokens.begin(), tokens.end());
  std::string rhost = base::JoinStrings(tokens, ".");
  CHECK_EQ(host.size(), rhost.size());

  reversed_url->append(protocol.data(), protocol.size());
  reversed_url->append("://");
  reversed_url->append(rhost);

  if (!port.empty()) {
    // 有 port 时, 一定有一个 冒号
    reversed_url->append(":");
    reversed_url->append(port.data(), port.size());
  }

  if (!user_pass.empty()) {
    // 有 user_pass 时, 一定有一个 @
    reversed_url->append("@");
    reversed_url->append(user_pass.data(), user_pass.size());
  }

  if (!path.empty()) {
    reversed_url->append(path.data(), path.size());
  }

  return true;
}

bool IsNewHbaseKey(const std::string &new_hbase_key) {
  base::Slice rurl(new_hbase_key);
  if (rurl.size() > 7 && rurl[6] == '-') {
    return true;
  }

  return false;
}

bool UpdateHbaseKey(const std::string &hbase_key, std::string *new_hbase_key) {
  base::Slice rurl(hbase_key);
  if (rurl.size() < 6 || rurl[5] != '-') {
    return false;
  }
  if (rurl.find('\0') != base::Slice::npos) {
    return false;
  }

  // old_key: 16384-http://...
  rurl.remove_prefix(6);
  uint64 hash = base::CityHash64WithSeed(rurl.data(), rurl.size(), kHashSeed);
  hash = hash & kInt64Max;  // 去掉 hash 值的最高位
  int shard_id = hash % kNewShardNum;
  *new_hbase_key = base::StringPrintf("%06d-", shard_id);
  new_hbase_key->append(rurl.data(), rurl.size());
  return true;
}

void GenNewHbaseKeyAndShardID(const std::string &url, std::string *hbase_key, int *shard_id) {
  std::string rurl;
  if (!ReverseUrl(url, &rurl)) {
    // 反转失败的, 直接用 url 本身
    rurl = url;
  }
  uint64 hash = base::CityHash64WithSeed(rurl.data(), rurl.size(), kHashSeed);
  hash = hash & kInt64Max;  // 去掉 hash 值的最高位
  *shard_id = hash % kNewShardNum;
  *hbase_key = base::StringPrintf("%06d-%s", *shard_id, rurl.c_str());
}

void GenNewHbaseKey(const std::string &url, std::string *hbase_key) {
  int shard_id = 0;
  GenNewHbaseKeyAndShardID(url, hbase_key, &shard_id);
}

void GenHbaseKey(const std::string &url, std::string *hbase_key) {
  std::string rurl;
  if (!ReverseUrl(url, &rurl)) {
    // 反转失败的, 直接用 url 本身
    rurl = url;
  }
  uint64 hash = base::CityHash64WithSeed(rurl.data(), rurl.size(), kHashSeed);
  hash = hash & kInt64Max;  // 去掉 hash 值的最高位
  int shard_id = hash % kShardNum;
  *hbase_key = base::StringPrintf("%05d-%s", shard_id, rurl.c_str());
}

void GenHbaseValue(const char *family, const std::string &value, std::string *hbase_value) {
  hbase_value->clear();
  CHECK_LT(strlen(family), 128u);
  char s = (char)strlen(family);
  *hbase_value = base::StringPrintf("%c%s", s, family);
  hbase_value->append(value);  // XXX(suhua): value 中可能包含 \0, 所以不能用 StringPrintf.
}

}  // namespace crawler2

