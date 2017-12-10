#include "crawler/proto2/proto_transcode.h"

#include <arpa/inet.h>
#include <string>

#include "base/common/logging.h"
#include "base/common/basic_types.h"
#include "base/hash_function/url.h"
#include "base/strings/string_printf.h"

namespace crawler2 {

static const int kCurrentVersion = -3;

void CrawledResourceToResource(const crawler::CrawledResource &input,
                               crawler2::Resource *result) {
  CHECK(input.IsInitialized());
  CHECK_NOTNULL(result);
  result->Clear();

  // 1. 填写 crawled content
  crawler2::Content *content = result->mutable_content();
  if (input.has_content_raw()) {
    content->set_raw_content(input.content_raw());
  }
  if (input.has_content_utf8()) {
    content->set_utf8_content(input.content_utf8());
  }

  // 2. 填写 parsed content
  crawler2::ParsedData *pc = result->mutable_parsed_data();
  pc->mutable_css_urls()->CopyFrom(input.css_urls());
  pc->mutable_image_urls()->CopyFrom(input.image_urls());

  // 3. 填写 graph
  // CrawledResource 暂时没有 graph 字段
  // TODO(suhua): 一旦 CrawledResource 里添加了 graph 相关字段，这里要相应添加
  // crawler2::Graph *graph = result->mutable_graph();

  // 4. 填写 alt
  // CrawledResource 暂时没有 alt 字段
  // TODO(suhua): 一旦 CrawledResource 里添加了 alt 相关字段，这里要相应添加
  // crawler2::Alt *alt = result->mutable_alt();

  // 5. 最麻烦的一步, 填写 brief
  crawler2::Brief *brief = result->mutable_brief();
  CHECK(input.has_url());
  brief->set_url(input.url());
  brief->set_source_url(input.source_url());
  brief->set_effective_url(input.effective_url());
  brief->set_resource_type((crawler2::ResourceType)(int)input.resource_type());
  if (input.has_http_header_raw()) {
    brief->set_response_header(input.http_header_raw());
  }
  if (input.has_original_encoding()) {
    brief->set_original_encoding(input.original_encoding());
  }
  if (input.has_is_truncated()) {
    brief->set_is_truncated(input.is_truncated());
  }
  if (input.has_crawler_info()) {
    const crawler::CrawlerInfo &src = input.crawler_info();
    crawler2::CrawlInfo *tar = brief->mutable_crawl_info();
    if (src.has_link_score()) {
      tar->set_score(src.link_score());
    }
    if (src.has_response_code()) {
      tar->set_response_code(src.response_code());
    }
    if (src.has_file_time()) {
      tar->set_file_timestamp(src.file_time());
    }
    if (src.has_crawler_timestamp()) {
      tar->set_crawl_timestamp(src.crawler_timestamp());
    }
    if (src.has_crawler_duration()) {
      tar->set_crawl_time(src.crawler_duration());
    }
    if (src.has_connect_time()) {
      tar->set_connect_time(src.connect_time());
    }
    if (src.has_appconnect_time()) {
      tar->set_appconnect_time(src.appconnect_time());
    }
    if (src.has_pretransfer_time()) {
      tar->set_pretransfer_time(src.pretransfer_time());
    }
    if (src.has_starttransfer_time()) {
      tar->set_starttransfer_time(src.starttransfer_time());
    }
    if (src.has_redirect_time()) {
      tar->set_redirect_time(src.redirect_time());
    }
    if (src.has_redirect_count()) {
      tar->set_redirect_times(src.redirect_count());
    }
    if (src.has_donwload_speed()) {
      tar->set_donwload_speed(src.donwload_speed());
    }
    if (src.has_content_length_donwload()) {
      tar->set_content_length_donwload(src.content_length_donwload());
    }
    if (src.has_header_size()) {
      tar->set_header_bytes(src.header_size());
    }
    if (src.has_content_type()) {
      tar->set_content_type(src.content_type());
    }
    if (src.has_update_fail_cnt()) {
      tar->set_update_fail_times(src.update_fail_cnt());
    }
    if (src.has_from()) {
      tar->set_from(src.from());
    }
  }
}

void ResourceToCrawledResource(const crawler2::Resource &input,
                               crawler::CrawledResource *result) {
  CHECK(input.IsInitialized());
  CHECK_NOTNULL(result);
  result->Clear();

  // 1. 转换 crawled content
  if (input.has_content()) {
    const crawler2::Content &content = input.content();
    if (content.has_raw_content()) {
      result->set_content_raw(content.raw_content());
    }
    if (content.has_utf8_content()) {
      result->set_content_utf8(content.utf8_content());
    }
  }

  // 2. 转换 parsed data
  if (input.has_parsed_data()) {
    const crawler2::ParsedData &pc = input.parsed_data();
    result->mutable_css_urls()->CopyFrom(pc.css_urls());
    result->mutable_image_urls()->CopyFrom(pc.image_urls());
  }

  // 3. 填写 graph
  // CrawledResource 暂时没有 graph 字段
  // TODO(suhua): 一旦 CrawledResource 里添加了 graph 相关字段，这里要相应添加
  // crawler2::Graph *graph = result->mutable_graph();

  // 4. 填写 alt
  // CrawledResource 暂时没有 alt 字段
  // TODO(suhua): 一旦 CrawledResource 里添加了 alt 相关字段，这里要相应添加
  // crawler2::Alt *alt = result->mutable_alt();

  // 5. 最麻烦的一步, 填写 brief
  if (input.has_brief()) {
    const crawler2::Brief &brief = input.brief();

    result->set_url(brief.url());
    result->set_source_url(brief.source_url());
    result->set_effective_url(brief.effective_url());
    result->set_resource_type((crawler::ResourceType)(int)brief.resource_type());

    if (brief.has_response_header()) {
      result->set_http_header_raw(brief.response_header());
    }
    if (brief.has_original_encoding()) {
      result->set_original_encoding(brief.original_encoding());
    }
    if (brief.has_is_truncated()) {
      result->set_is_truncated(brief.is_truncated());
    }
    if (brief.has_crawl_info()) {
      const crawler2::CrawlInfo src = brief.crawl_info();
      crawler::CrawlerInfo *tar = result->mutable_crawler_info();
      if (src.has_score()) {
        tar->set_link_score(src.score());
      }
      if (src.has_response_code()) {
        tar->set_response_code(src.response_code());
      }
      if (src.has_file_timestamp()) {
        tar->set_file_time(src.file_timestamp());
      }
      if (src.has_crawl_timestamp()) {
        tar->set_crawler_timestamp(src.crawl_timestamp());
      }
      if (src.has_crawl_time()) {
        tar->set_crawler_duration(src.crawl_time());
      }
      if (src.has_connect_time()) {
        tar->set_connect_time(src.connect_time());
      }
      if (src.has_appconnect_time()) {
        tar->set_appconnect_time(src.appconnect_time());
      }
      if (src.has_pretransfer_time()) {
        tar->set_pretransfer_time(src.pretransfer_time());
      }
      if (src.has_starttransfer_time()) {
        tar->set_starttransfer_time(src.starttransfer_time());
      }
      if (src.has_redirect_time()) {
        tar->set_redirect_time(src.redirect_time());
      }
      if (src.has_redirect_times()) {
        tar->set_redirect_count(src.redirect_times());
      }
      if (src.has_donwload_speed()) {
        tar->set_donwload_speed(src.donwload_speed());
      }
      if (src.has_content_length_donwload()) {
        tar->set_content_length_donwload(src.content_length_donwload());
      }
      if (src.has_header_bytes()) {
        tar->set_header_size(src.header_bytes());
      }
      if (src.has_content_type()) {
        tar->set_content_type(src.content_type());
      }
      if (src.has_update_fail_times()) {
        tar->set_update_fail_cnt(src.update_fail_times());
      }
      if (src.has_from()) {
        tar->set_from(src.from());
      }
    }
  }
}

bool HbaseDumpToResource(const char *data, int bytes, crawler2::Resource *result) {
  CHECK_NOTNULL(result);
  result->Clear();

  // 新版本的前 4 个字节是整数 -2, 且第一个字段是 hint, 用于辅助合并网页,
  // 在转换成 Resource 时可以忽略
  if (bytes < 8) {
    DLOG(ERROR) << "data should be 8 bytes at least";
    return false;
  }
  {
    // 检查是否是版本 -2 或者 最新版本 -3, 如果是, 则忽略第一个字段
    int version = *reinterpret_cast<const int *>(data);
    if (version == -2 || version == -3) {
      int hint_size = *reinterpret_cast<const int *>(data + 4);
      // 8 = sizeof(version) + sizeof(hint_size)
      data = data + 8 + hint_size;
      bytes = bytes - 8 - hint_size;
    }
  }

  // data 的格式是, 一共 5 个部分, 每个部分包括: 1. 先是 4 字节表示内容长度, 2. 接下来是内容
  // 5 个部分依次是 brief, content, parsed_data, graph, alt 的 proto 的序列化结果
  // 长度为 0 表示没有值

  // 5 个部分的长度字段, 至少要有 20 个字节
  if (bytes < 20) {
    DLOG(ERROR) << "data should be 20 bytes at least";
    return false;
  }

  // 解析出每个部分的长度, 以及数据地址
  const int kPartNum = 5;
  int parts_size[kPartNum];
  const char *parts_data[kPartNum];

  const char *p = data;
  for (int i = 0; i < kPartNum; ++i) {
    if (p - data + 4 > bytes) return false;
    parts_size[i] = *reinterpret_cast<const int *>(p);
    if (parts_size[i] == 0) {
      // 表示该字段设置为 空
    } else if (parts_size[i] == -1) {
      // -1 表示该字段不设置
    } else if (parts_size[i] < -1) {
      return false;
    }
    p += 4;
    parts_data[i] = p;
    p += (parts_size[i] == -1? 0: parts_size[i]);
  }
  if (p - data != bytes) return false;

  // 将解析出来的数据, 填充到 proto 中去
  //
  // 对应字段长度 > 0, 则添加一个字段, 从数据中解析值
  // 对应字段长度 == 0, 则添加一个空的字段
  // 对应字段长度 == -1, 则不添加字段
  if (parts_size[0] > 0) {
    if (!result->mutable_brief()->ParseFromArray(parts_data[0], parts_size[0])) return false;
  } else if (parts_size[0] == 0) {
    result->mutable_brief();
  } else {
    CHECK_EQ(parts_size[0], -1);
  }

  if (parts_size[1] > 0) {
    if (!result->mutable_content()->ParseFromArray(parts_data[1], parts_size[1])) return false;
  } else if (parts_size[1] == 0) {
    result->mutable_content();
  } else {
    CHECK_EQ(parts_size[1], -1);
  }

  if (parts_size[2] > 0) {
    if (!result->mutable_parsed_data()->ParseFromArray(parts_data[2], parts_size[2])) return false;
  } else if (parts_size[2] == 0) {
    result->mutable_parsed_data();
  } else {
    CHECK_EQ(parts_size[2], -1);
  }

  if (parts_size[3] > 0) {
    if (!result->mutable_graph()->ParseFromArray(parts_data[3], parts_size[3])) return false;
  } else if (parts_size[3] == 0) {
    result->mutable_graph();
  } else {
    CHECK_EQ(parts_size[3], -1);
  }

  if (parts_size[4] > 0) {
    if (!result->mutable_alt()->ParseFromArray(parts_data[4], parts_size[4])) return false;
  } else if (parts_size[4] == 0) {
    result->mutable_alt();
  } else {
    CHECK_EQ(parts_size[4], -1);
  }

  return true;
}

void ResourceToHbaseDump(const crawler2::Resource &res, std::string *str) {
  CHECK_NOTNULL(str);

  LOG_IF(FATAL, !res.IsInitialized()) << "Incomplete protobuf: " << res.InitializationErrorString();

  str->clear();
  int minus_one = -1;

  static_assert(sizeof(minus_one) == 4u, "sizeof(int) should be 4!");

  // 1. 前 4 字节如果小于 -1, 则表示 hbasedump 格式的版本号
  // 目前是 -2 版本, 兼容旧版本 (旧版本的地前四个字节是大于等于 -1 的)
  str->append(reinterpret_cast<const char *>(&kCurrentVersion), sizeof(kCurrentVersion));

  CHECK(res.has_brief());

  // 版本 2 的第一个字段是 hint, 用于指导相同网页的合并
  {
    auto &br = res.brief();
    bool is_30X = (br.source_url() != br.effective_url());
    uint64 src_url_sign = base::CalcUrlSign(br.source_url().data(), br.source_url().size());
    double uv_rank = 0;
    int64 uv_timestamp_in_sec = 0;
    double click_rank = 0;
    int64 click_timestamp_in_sec = 0;
    int64 crawl_timestamp_in_sec = 0;
    int64 response_code = 0;

    if (res.has_graph()) {
      if (res.graph().has_uv_rank()) {
        uv_rank = res.graph().uv_rank().rank_score();
        uv_timestamp_in_sec = res.graph().uv_rank().timestamp_in_sec();
      }
      if (res.graph().has_click_rank()) {
        click_rank = res.graph().click_rank().rank_score();
        click_timestamp_in_sec = res.graph().click_rank().timestamp_in_sec();
      }
    }
    if (br.has_crawl_info()) {
      if (br.crawl_info().has_crawl_timestamp()) {
        crawl_timestamp_in_sec = br.crawl_info().crawl_timestamp() / 1000 / 1000;
      }
      if (br.crawl_info().has_response_code()) {
        response_code = br.crawl_info().response_code();
      }
    }

    std::string hint = base::StringPrintf(
        "%d\t%lu\t%g\t%ld\t%g\t%ld\t%ld\t%ld",
        is_30X, src_url_sign,
        uv_rank, uv_timestamp_in_sec,
        click_rank, click_timestamp_in_sec,
        crawl_timestamp_in_sec,
        response_code);

    int size = (int)hint.size();
    str->append(reinterpret_cast<char *>(&size), sizeof(size));
    str->append(hint);
  }

  // 第二个字段是 brief
  if (res.has_brief()) {
    int size = res.brief().ByteSize();
    str->append(reinterpret_cast<char *>(&size), sizeof(size));
    CHECK(res.brief().AppendToString(str));
  } else {
    // 填写 -1 表示该字段保留网页库原始值
    str->append(reinterpret_cast<char *>(&minus_one), sizeof(minus_one));
  }

  // 第三个字段
  if (res.has_content()) {
    int size = res.content().ByteSize();
    str->append(reinterpret_cast<char *>(&size), sizeof(size));
    CHECK(res.content().AppendToString(str));
  } else {
    // 填写 -1 表示该字段保留网页库原始值
    str->append(reinterpret_cast<char *>(&minus_one), sizeof(minus_one));
  }

  // 第四个字段
  if (res.has_parsed_data()) {
    int size = res.parsed_data().ByteSize();
    str->append(reinterpret_cast<char *>(&size), sizeof(size));
    CHECK(res.parsed_data().AppendToString(str));
  } else {
    // 填写 -1 表示该字段保留网页库原始值
    str->append(reinterpret_cast<char *>(&minus_one), sizeof(minus_one));
  }

  // 第五个字段
  if (res.has_graph()) {
    int size = res.graph().ByteSize();
    str->append(reinterpret_cast<char *>(&size), sizeof(size));
    CHECK(res.graph().AppendToString(str));
  } else {
    // 填写 -1 表示该字段保留网页库原始值
    str->append(reinterpret_cast<char *>(&minus_one), sizeof(minus_one));
  }

  // 第六个字段
  if (res.has_alt()) {
    int size = res.alt().ByteSize();
    str->append(reinterpret_cast<char *>(&size), sizeof(size));
    CHECK(res.alt().AppendToString(str));
  } else {
    // 填写 -1 表示该字段保留网页库原始值
    str->append(reinterpret_cast<char *>(&minus_one), sizeof(minus_one));
  }
}

}  // namespace crawler2

