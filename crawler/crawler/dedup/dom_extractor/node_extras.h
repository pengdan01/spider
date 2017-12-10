#pragma once

#include <iostream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>

#include "base/common/base.h"
#include "web/html/api/parser.h"

namespace dom_extractor {
struct NodeID {
  web::HTMLTag::TagType tag_type;
  std::string tag_name;
  std::string class_name;
  std::string id_name;
  std::string full_name;
  std::string path;  // 从根节点 (body) 到当前节点的路径名
};
class NodeExtras {
 public:
  NodeExtras();
  ~NodeExtras();
  void Clear();
  void SetCounterZero();
  void UpdateDensity();
  void AddChild(const NodeExtras &child_extras);

  std::string text;              // 如果是纯文本节点, 记录文本内容
  int char_num;                  // 字符数, 包含超链接文字
  int nonlink_char_num;          // 非超链接文字
  int punctuation_num;           // 标点符号数
  int tag_num;                   // 标签数
  int link_char_num;             // 超链接字符数
  int link_tag_num;              // 超链接个数
  int tag_h_num;                 // h 标签数
  int tag_p_num;                 // p 标签数

  float text_density;
  float punctuation_density;
  float tag_h_density;
  float tag_p_density;
  float link_char_density;
  float link_tag_density;
  float nonlink_char_body_ratio;          // 当前节点非超链接文字与本页面所有非超链接文字的比例
  bool has_text;
  bool is_main_content;
  int indent;
  NodeID node_id;
};
}
