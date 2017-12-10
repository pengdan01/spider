#include "crawler/api/base.h"

#include <set>
#include <vector>
#include <fstream>

#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"
#include "base/strings/string_number_conversions.h"
#include "base/hash_function/fast_hash.h"
#include "web/url/url.h"
#include "crawler/proto/crawled_resource.pb.h"

DEFINE_string(crawler_tld_file, "crawler/api/tld.dat", "file path of tld.dat");

namespace crawler {

bool NormalizeUrl(const std::string &url, std::string *normalized_url) {
  std::string myurl(url);
  base::TrimWhitespaces(&myurl);
  // 去掉前面的 %20%5E 等信息
  while (myurl[0] == '%' && myurl.size() >= 3) myurl = myurl.substr(3);
  if (myurl.empty()) {
    LOG(WARNING) << "url is empty or only contains strings like %20%5E and so on, url: " << url;
    return false;
  }
  if ((int)myurl.size() > crawler::kMaxLinkSize) {
    LOG(WARNING) << "url exceeds max url lenght limit(" << crawler::kMaxLinkSize << "), url: " << url;
    return false;
  }
  // 忽略掉 '#' 以及后面的所有参数.
  // NOTE: google 比较特殊, 存在大量类似如下的 url:
  // http://www.google.com.hk/#hl=zh-CN&newwindow=1&safe=strict&site=&q=flower&oq=flower&aq=f&aqi=&aql=
  // 因此, 对此类 url 不进行该处理
  std::string::size_type pos = myurl.find('#');
  if (pos != std::string::npos) {
    if (!(base::StartsWith(myurl, "http://www.google.com", false) ||
          base::StartsWith(myurl, "www.google.com", false))) {
      myurl = myurl.substr(0, pos);
    }
  }
  // 使用 http 补全 url 的 schema
  /*
  pos = myurl.find(":");
  if (pos == std::string::npos) {
  */
  if (!base::StartsWith(myurl, "http:", false) && !base::StartsWith(myurl, "https:", false)  &&
      !base::StartsWith(myurl, "javascript:", false) && !base::StartsWith(myurl, "mailto:", false) &&
      !base::StartsWith(myurl, "ftp:", false)  && !base::StartsWith(myurl, "data:", false)   &&
      !base::StartsWith(myurl, "gopher:", false) && !base::StartsWith(myurl, "file:", false)) {
    myurl = "http://" + myurl;
  }
  GURL gurl(myurl);
  if (!gurl.is_valid()) {
    LOG(WARNING) << "url not pass the check of gurl.is_valid(), url: " << url;
    return false;
  }
  *normalized_url = gurl.spec();
  return true;
}

bool AssignShardId(const std::string &url, int32 shard_num, int32 *shard_id, bool switch_rawtoclick_url) {
  CHECK_GT(shard_num, 0);
  CHECK_NOTNULL(shard_id);
  std::string click_url(url);
  if (switch_rawtoclick_url == true) {
    if (!web::RawToClickUrl(url, NULL, &click_url)) {
      LOG(WARNING) << "Faild in :RawToClickUrl(), url: " << url;
      return false;
    }
  }
  *shard_id = base::CityHash64(click_url.data(), click_url.size()) % shard_num;
  return true;
}

bool AssignReduceId(const std::string &url, int32 reduce_task_num, int32 *reduce_id,
                    bool switch_rawtoclick_url) {
  CHECK_GT(reduce_task_num, 0);
  CHECK_NOTNULL(reduce_id);
  std::string click_url(url);
  if (switch_rawtoclick_url == true) {
    if (!web::RawToClickUrl(url, NULL, &click_url)) {
      LOG(WARNING) << "Faild in :RawToClickUrl(), url: " << url;
      return false;
    }
  }
  std::string output(click_url.rbegin(), click_url.rend());
  *reduce_id = base::CityHash64(output.data(), output.size()) % reduce_task_num;
  return true;
}

bool ReverseUrl(const std::string &url, std::string *reverted_url, bool switch_rawtoclick_url) {
  CHECK_NOTNULL(reverted_url);
  std::string click_url(url);
  if (switch_rawtoclick_url == true) {
    if (!web::RawToClickUrl(url, NULL, &click_url)) {
      LOG(WARNING) << "Faild in :RawToClickUrl(), url: " << url;
      return false;
    }
  }
  GURL gurl(click_url);
  const std::string &host = gurl.host();
  if (host.empty()) {
    LOG(WARNING) << "Invalid url(gurl.host() is empty), url: " << url;
    return false;
  }
  std::vector<std::string> tokens;
  int num = base::Tokenize(host, ".", &tokens);
  // 判断 host 是否为 IP: 如果是 IP, 则不对其进行反转
  if (num == 4) {
    bool is_ip = true;
    int tmp;
    for (int i = 0; i < num && is_ip; ++i) {
      if (!base::StringToInt(tokens[i], &tmp)) is_ip = false;
    }
    if (is_ip == true) {
      *reverted_url = url;
      return true;
    }
  }
  std::string rhost;
  for (int i = num-1; i >=0; --i) {
    if (i == 0) {
      rhost = rhost + tokens[i];
    } else {
      rhost = rhost + tokens[i] + ".";
    }
  }
  const std::string &spec =  gurl.spec();
  std::string::size_type pos = spec.find(host);
  std::string other = spec.substr(pos + host.size());
  if (gurl.username().empty() && gurl.password().empty()) {
    *reverted_url = gurl.scheme() + "://" + rhost + other;
  } else {
    *reverted_url = gurl.scheme() + "://" + gurl.username() + ":" + gurl.password() + "@" + rhost + other;
  }
  return true;
}

static const std::set<std::string> *CreateTLDSet() {
  CHECK(!FLAGS_crawler_tld_file.empty());
  std::set<std::string> *tld_set = new std::set<std::string>();
  std::ifstream fin(FLAGS_crawler_tld_file);
  LOG_IF(FATAL, !fin.good()) << "Failed in open domain dict file: " << FLAGS_crawler_tld_file
                             << ", using gflag --crawler_tld_file to specified it.";
  std::string line;
  while (getline(fin, line)) {
    base::TrimWhitespaces(&line);
    if (line.empty()) continue;
    tld_set->insert(line);
  }
  CHECK(fin.eof());
  LOG(INFO) << "tld loaded: " << tld_set->size() << " records";
  return tld_set;
}

static const std::set<std::string> *GetTLDSet() {
  // XXX(suhua): gcc will guarantee it's thread safe
  // this is standard single-ton pattern.
  static const std::set<std::string> *tld = CreateTLDSet();
  return tld;
}

bool ParseHost(const std::string &host, std::string *tld, std::string *domain, std::string *subdomain) {
  CHECK(!(tld == NULL && domain == NULL && subdomain == NULL));
  std::string webhost = host;
  base::TrimWhitespaces(&webhost);
  if (webhost.empty()) {
    LOG(WARNING) << "Invalid host: only contains white spaces, host: " << host;
    return false;
  }
  base::LowerString(&webhost);
  const std::set<std::string> *tld_dict = GetTLDSet();
  int64 pos0 = -1;
  std::string::size_type pos = webhost.find(".");
  while (pos != std::string::npos) {
    if ((int)pos == pos0+1) {
      LOG(WARNING) << "Invalid host: two or more continous dot, host: " << host;
      return false;
    }
    std::string tmp_domain = webhost.substr(pos+1);
    auto it = tld_dict->find(tmp_domain);
    if (it == tld_dict->end()) {
      pos0 = pos;
      pos = webhost.find(".", pos+1);
    } else {  // find domain
      if (tld != NULL) *tld = *it;
      if (domain != NULL) *domain = webhost.substr(pos0+1);
      if (subdomain != NULL) {
        if (pos0 <= 0) {
          *subdomain = "";
        } else {
          *subdomain = webhost.substr(0, pos0);
        }
      }
      return true;
    }
  }
  return false;
}
}  // namespace crawler

