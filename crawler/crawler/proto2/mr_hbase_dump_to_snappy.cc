#include <vector>
#include <algorithm>
#include <utility>

#include "base/common/base.h"
#include "base/common/logging.h"
#include "base/mapreduce/mapreduce.h"
#include "base/zip/snappy.h"
#include "base/strings/string_printf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_codepage_conversions.h"
#include "crawler/proto2/resource.pb.h"
#include "crawler/proto2/proto_transcode.h"
#include "crawler/proto2/gen_hbase_key.h"
#include "web/html/api/html_parser.h"

bool FillParsedData(crawler2::Resource *resource) {
  crawler2::Resource &res = *resource;

  if (!res.has_content()
      || !res.content().has_raw_content()) {
    // 这种, 没有 raw_content 的一般有 snappy_raw_content,
    // 已经是新版本的了, 其 parsed_data 已经填充过了
    return true;
  }

  if (res.has_parsed_data()
      && res.parsed_data().css_urls_size() > 0) {
    // 已经有 css urls 了, 不用再解析
    return true;
  }

  // 1. 分析
  // 转码, 提取 css/image 链接, 并填写到 Resource 中
  std::string utf8_html;

  int skipped_bytes;
  int nonascii_bytes;
  const char *code_page = base::ConvertHTMLToUTF8WithBestEffort(
      res.brief().effective_url(), res.brief().response_header(), res.content().raw_content(),
      &utf8_html, &skipped_bytes, &nonascii_bytes);

  if (code_page == NULL) {
    return false;
  }

  // XXX(suhua): 注意, 这里网页的 url 需要用 effective url 而不是 source url.
  web::HTMLDoc doc(utf8_html.c_str(), res.brief().effective_url().c_str());
  web::HTMLParser parser;
  parser.Init();
  parser.Parse(&doc, web::HTMLParser::kParseSpecialNodes);

  {
    std::set<std::string> css_urls;
    for (auto it = doc.css_links().begin(); it != doc.css_links().end(); ++it) {
      if (css_urls.find(*it) != css_urls.end()) continue;
      css_urls.insert(*it);
      res.mutable_parsed_data()->add_css_urls(*it);
    }
    VLOG(3) << css_urls.size() << " css urls in: " << res.brief().effective_url();
  }

  {
    std::set<std::string> image_urls;
    for (auto it = doc.image_links().begin(); it != doc.image_links().end(); ++it) {
      if (image_urls.find(it->url) != image_urls.end()) continue;
      image_urls.insert(it->url);
      res.mutable_parsed_data()->add_image_urls(it->url);
    }
  }

  return true;
}

void Mapper() {
  auto task = base::mapreduce::GetMapTask();

  std::string key;
  std::string value;

  std::string output_key;
  std::string output_value;
  crawler2::Resource res;

  int64 num = 0;
  int64 raw_bytes = 0;
  int64 snappy_bytes = 0;

  int64 bad_num = 0;
  int64 bad_raw_bytes = 0;
  int64 bad_snappy_bytes = 0;

  while (task->NextKeyValue(&key, &value)) {
    // key:  16384-http://...
    if (key.size() < 6 || key[5] != '-') {
      LOG(WARNING) << "ignore invalid key: " << key;
      ++bad_num;
      bad_raw_bytes += value.size();
      continue;
    }

    res.Clear();
    CHECK(crawler2::HbaseDumpToResource(value.data(), value.size(), &res));

    bool is_bad = false;
    if (res.has_content() && res.content().has_raw_content()) {
      // 把 css urls & image urls 提取出来, 填到 resource 中去
      if (!FillParsedData(&res)) {
        is_bad = true;
      }

      base::Slice s = res.brief().crawl_info().content_type();
      if (is_bad
          && s.find_case_insensitive("text") == base::Slice::npos
          && s.find_case_insensitive("html") == base::Slice::npos
          // && (s.find_case_insensitive("application") != base::Slice::npos
          //     || s.find_case_insensitive("image") != base::Slice::npos
          //     || s.find_case_insensitive("video") != base::Slice::npos
          //     )
          ) {
        // 无法转 utf-8, 且 content_type 里没有 text 和 html, 但是有
        // application 肯定不是网页, 这里就不压缩了 后面还会清除 raw_content 和
        // utf8_content, 相当于 url 入库, 但是内容清空
      } else {
        // 生成压缩过的 snappy_raw_content();
        base::snappy::Compress(res.content().raw_content().data(), res.content().raw_content().size(),
                               res.mutable_content()->mutable_snappy_raw_content());
      }

      // 清除 proto 中的 raw_content();
      res.mutable_content()->clear_raw_content();
      // 清除 proto 中的 utf8_content();
      res.mutable_content()->clear_utf8_content();
    }
    output_value.clear();
    crawler2::ResourceToHbaseDump(res, &output_value);

    if (!crawler2::IsNewHbaseKey(key)) {
      output_key.clear();
      if (!crawler2::UpdateHbaseKey(key, &output_key)) {
        output_key = key;
      }
    } else {
      output_key = key;
    }

    task->Output(output_key, output_value);

    ++num;
    raw_bytes += value.size();
    snappy_bytes += output_value.size();

    if (is_bad) {
      ++bad_num;
      bad_raw_bytes += value.size();
      bad_snappy_bytes += output_value.size();
      LOG(WARNING) << "found bad url,\tcontent_type:\t" << res.brief().crawl_info().content_type()
                   << "\tsrc_url:\t" << res.brief().source_url();
    } else {
      VLOG(1) << "found good url,\tcontent_type:\t" << res.brief().crawl_info().content_type()
              << "\tsrc_url:\t" << res.brief().source_url();
    }
  }

  LOG(INFO) << "num: " << num
            << ", raw_bytes: " << raw_bytes
            << ", snappy_bytes: " << snappy_bytes;
  LOG(INFO) << "bad num: " << bad_num
            << ", bad_raw_bytes: " << bad_raw_bytes
            << ", bad_snappy_bytes: " << bad_snappy_bytes;
}

int main(int argc, char **argv) {
  base::mapreduce::InitMapReduce(&argc, &argv, "encoding");
  CHECK(base::mapreduce::IsMapApp());
  Mapper();
}

