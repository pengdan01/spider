#include "crawler/dedup/dom_extractor/node_extras.h"

#include <iostream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>

#include "base/common/base.h"
#include "base/common/slice.h"
#include "base/file/file_path.h"
#include "base/file/file_util.h"
#include "nlp/common/nlp_util.h"
#include "base/strings/utf_char_iterator.h"
#include "base/strings/utf_codepage_conversions.h"
#include "base/strings/string_util.h"
#include "base/testing/gtest.h"
#include "web/html/api/parser.h"

namespace dom_extractor {
NodeExtras::NodeExtras() {
  has_text = false;
  indent = -1;
  is_main_content = true;
  text = "";
  SetCounterZero();
}
NodeExtras::~NodeExtras() {
}
void NodeExtras::Clear() {
  SetCounterZero();
  has_text = true;
  indent = -1;
  is_main_content = true;
  text = "";
}
void NodeExtras::SetCounterZero() {
  char_num = 0;
  tag_num = 0;
  link_char_num = 0;
  link_tag_num = 0;
  punctuation_num = 0;
  tag_h_num = 0;
  tag_p_num = 0;
}
void NodeExtras::UpdateDensity() {
  text_density = char_num / (tag_num + 1.0);
  punctuation_density = punctuation_num / (char_num + 1.0);
  tag_h_density = tag_h_num / (tag_num + 1.0);
  tag_p_density = tag_p_num / (tag_num + 1.0);

  link_char_density = link_char_num / (char_num + 1.0);
  link_tag_density = link_tag_num / (tag_num + 1.0);
  return;
}

void NodeExtras::AddChild(const NodeExtras &child_extras) {
  char_num += child_extras.char_num;
  tag_num += child_extras.tag_num;
  link_char_num += child_extras.link_char_num;
  link_tag_num += child_extras.link_tag_num;
  punctuation_num += child_extras.punctuation_num;
  tag_h_num += child_extras.tag_h_num;
  tag_p_num += child_extras.tag_p_num;
}
}
