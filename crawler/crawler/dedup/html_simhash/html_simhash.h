#pragma once
// 为 HTML 文档计算 simhash 签名的类, 计算分为下几个部分
// 0. 文档预处理, 包括: 网页解析, 网页标题和正文文本抽取, 文本行规范化 (去除空格换行等), 分词, 本步结束
//    将获得一系列 term
// 1. 文档特征（向量） 化,为每一个出现在字典 (白名单) 中而没有出现在 stop words (黑名单) 中的 term 都赋
//    一个权重, 目前的实现方案中权重取 term 的 tf * idf
// 2. 特征签名, 为每一个 term 计算一个 hash 签名
// 3. 通过特征签名 以及 特征权重 计算 simhash

// NOTE(yuanjinhui): 需要 url tld.dat 文件
// 1. 通过 --url_tld_file 指定 tld.dat 的本地路径, ParseHost() 需要使用到该词表文件;
//   --url_tld_file 默认取值 web/url/tld.dat
// 2. 如果调用模块在 hadoop 上运行, 则在提交 hadoop 任务参数中, 需要额外添加如下参数:
// --mr_cache_archives /wly/web.tar.gz#web(若 --url_tld_file 使用默认值)

// NOTE(yuanjinhui): 需要 bad_hash 文件
// 1. 通过 --bad_hash 指定 bad_hash 的本地路径, 默认取值是 crawler/dedup/html_simhash/data/bad_hash
// 2, 如果调用模块在 hadoop 上运行, 则在提交 hadoop 任务参数中, 需要额外添加如下参数:
//    --mr_cache_archives /wly/crawler.tar.gz#crawler (若--bad_hash 使用默认值)

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

#include "base/common/basic_types.h"
#include "nlp/common/nlp_util.h"
#include "nlp/idf/idf.h"
#include "nlp/segment/segmenter.h"
#include "crawler/dedup/dom_extractor/content_collector.h"

namespace nlp {
namespace segment {
class Segmenter;
}
}

namespace simhash {
typedef unsigned HashMode;
struct PageSignature {
  PageSignature() {
    domain_hash_ = 0u;
    title_hash_ = 0u;
    content_hash_ = 0u;
  }
  uint64 domain_hash_;
  uint64 title_hash_;
  uint64 content_hash_;
};

class HtmlSimHash {
 public:
  HtmlSimHash() {
    hash_mode_ = (HashMode) kHashAll;
  }
  ~HtmlSimHash() {
  }
  enum HashModes {
    kHashDomain        = 0x00000001,  // 为域名计算签名
    kHashHeadTitle     = 0x00000002,  // 为 html head title 计算签名
    kHashDomainTitle   = 0x00000003,  // 为域名和 head title 计算签名
    kHashMainContent   = 0x00000004,  // 为正文计算签名
    kHashTitleContent  = 0x00000006,  // 为 html head title 和正文计算签名
    kHashAll           = 0x00000007,  // 为域名, html head title, 正文计算签名
  };
  HashMode hash_mode() const {
    return hash_mode_;
  }

  // 1, 在调用 CalculateSimHash 之前, 可调用 Init 设置计算 Hash 签名的模式,
  // 若不调用 Init 默认计算模式是 kHashAll
  // 2, 调用 VerifyBadPage 和 VerifyBadSignature 之前必须调用 Init, 且 mode 必须是 kHashAll
  bool Init(const HashModes &mode);
  // 为地址为 |url| 的网页 |page_utf8| 计算复合签名 (含域名, 标题, 正文), 结果保存在 |signature| 中
  // |page_utf8| 是转码成 utf8 的网页源文件
  // |url| 是网址, 如果计算签名模式 kHashDomain 为真, 则 |url| 不可以为空
  // 调用成功返回 true, 反之表示调用失败, 参数 |signature| 无意义
  bool CalculatePageHash(const std::string &page_utf8, const std::string &url, PageSignature *signature);

  // 为地址是 |url| 的网页 |page_utf8| 计算复合签名 (含域名, 标题, 正文), 结果保存在 |signature| 中
  // 并判断该网页是否是无效页面, is_bad == true 表示当前页面是无效页面
  // |page_utf8| 是转码成 utf8 的网页源文件
  // NOTE(yuanjinhui): |url| 必须是 RawToClick 转换后的 url,
  // 如果计算签名模式 kHashDomain 为真, 则 |url| 不可以为空
  // 调用成功返回 true, 反之表示调用失败, 参数 |signature| 和 |is_bad| 无意义
  bool VerifyBadPage(const std::string &page_utf8,
                         const std::string &url,
                         PageSignature *signature,
                         bool *is_bad);
  // 当已为网页计算过签名时推荐使用如下函数, |signature| 是网页签名, |url| 是网页地址, |is_bad| 返回该网页是否
  // 是无效网页, is_bad == true 表示当前网页是无效网页
  // 调用成功返回 true, 反之表示调用失败, 参数 |is_bad| 无意义
  bool VerifyBadSignature(const PageSignature &signature, const std::string &url, bool *is_bad);
  // 为地址为 |url| 的网页 |page_utf8| 计算正文 simhash 签名, 结果保存在 |hash| 中
  // |page_utf8| 是转码为 utf8 的网页 html 源文件
  // |url| 是网页的网址, 可以为空串
  // 调用成功返回 true, 反之表示调用失败, 参数 |hash| 无意义
  bool CalculateSimHash(const std::string &page_utf8, const std::string &url, uint64 *hash);
  // 当已经为网页抽取出 term count 时可调用下面的函数
  bool CalculateSimHashFromTermCount(const std::vector<std::pair<std::string, int64> > &term_count,
                                     const std::string &url,
                                     uint64 *hash);

  // 提取源文件为 |page_utf8| 的网页中出现的 term 及其出现次数
  // NOTE(yuanjinhui): 调用这个函数之前不需要调用 Init
  bool ExtractHtmlTitleContentTermCount(const std::string &page_utf8, const std::string &url,
                                        std::vector<std::pair<std::string, int64> > *term_count);

  bool CalcDomainHash(const std::string &url, uint64 *hash);
  bool CalcTitleHash(const std::string &head_utf8, uint64 *hash);

  std::string title() const {
    return title_;
  }
  std::string content() const {
    return content_;
  }

 private:
  bool AcceptLanguage(const std::string &term);

  // 把 |term_count| 中的每个 term 转化为 hash 值与 tf * idf 权重的序对
  bool ConstructTokenHashPairs(const std::vector<std::pair<std::string, int64> > &term_count,
                               std::vector<std::pair<uint64, double> > *token_hash_pairs);

  bool ExtractContentTermCount(const std::string &content,
                                        std::vector<std::pair<std::string, int64> > *term_count);
  nlp::segment::Segmenter segmenter_;
  nlp::idf::IdfDicts dicts_;
  dom_extractor::ContentCollector content_collector_;
  HashMode hash_mode_;
  std::unordered_map<std::string, std::string> bad_page_;

  std::string title_;
  std::string content_;

  DISALLOW_COPY_AND_ASSIGN(HtmlSimHash);

};
}  // namespace simhash
