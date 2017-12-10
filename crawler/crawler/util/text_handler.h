#pragma once

#include <vector>
#include <string>

#include "extend/regexp/re3/re3.h"

namespace crawler {

using extend::re3::Re3;

class TextHandler {
 public:
  TextHandler() {}
  ~TextHandler() {}
  // 将整个文件使用 Cescape 库进行转义，即： \n,\r,\t," 以及其他非 ascii 字符会重新编码
  // 输入： |fname| 文件名
  // 输出： |str| 转义后的字符串保存在 str 中
  // 返回 ：当 fname 为空或长度为 0 时，直接异常退出
  static bool CescapeWholeFile(const char *fname, std::string *str);

  // just different escape function:
  static bool CescapeWholeFileUtf8Safe(const char *fname, std::string *str);

  // 反转义，函数 CescapeWholeFile() 的逆操作
  // 输入： str 转义后的字符串
  // 输出：执行反转义后的输出到该文件
  // 返回 ：当 fname 为空或长度为 0 时，直接退出 返回 false
  static bool UnCnescapeWholeFile(const std::string &str, const char *fname);

  // a pair of CescapeWholeFileUtf8Safe()
  static bool UnCnescapeWholeFileUtf8Safe(const std::string &str, const char *fname);

  // 从 string 中提取 html 包含的 outlink
  static void ExtractLink(const std::string &link, const std::string &source,
                          std::vector<std::string> *html, const extend::re3::Re3 &pattern);
  // 从 string 中提取 css 对应的 link
  // link 为 page 对应的链接，之所以需要这个参数，是为了处理相对 URL 的情况
  static void ExtractCss(const std::string &link, const std::string &source, std::vector<std::string> *css);
  // 从 string 中提取 url 对应的 link
  static void ExtractImg(const std::string &link, const std::string &source, std::vector<std::string> *img);
  // 从 string 中提取 url 对应的 link
  static void ExtractHtml(const std::string &link, const std::string &source, std::vector<std::string> *url);
  static void Extract_Html_Css_Image(const std::string & link, const std::string &source,
                                     std::vector<std::string> *html, std::vector<std::string> *css,
                                     std::vector<std::string> *image);
 private:
};  // end class
}  // end namespace
