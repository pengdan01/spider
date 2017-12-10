#include "crawler/dedup/dom_extractor/node_cluster.h"
#include <vector>
#include <set>
#include <string>

#include "base/common/base.h"
#include "web/html/api/parser.h"
#include "base/strings/utf_char_iterator.h"
#include "crawler/dedup/dom_extractor/node_extras.h"

namespace dom_extractor {
NodeCluster::NodeCluster() {
  parent_ = NULL;
  first_child_ = NULL;
  next_sibling_ = NULL;
  path_ = "";
  indent_ = -1;
}
NodeCluster::~NodeCluster() {
  parent_ = NULL;
  first_child_ = NULL;
  next_sibling_ = NULL;
}
void NodeCluster::Clear() {
  path_ = "";
  indent_ = -1;
  dom_tree_nodes_.clear();
  parent_ = NULL;
  first_child_ = NULL;
  next_sibling_ = NULL;

  char_num = 0;
  tag_num = 0;
  link_char_num = 0;
  link_tag_num = 0;
  punctuation_num = 0;
  tag_h_num = 0;
  tag_p_num = 0;
  nonlink_char_num = 0;
  node_num = -1;
  text_length = 0;
  distinct_char_num = 0;
  text_length_per_node = 0;
  char_perplexity = 0;

  is_main_content = true;
}

void NodeCluster::CalcClusterFeature() {
  for (int id = 0; id < (int) dom_tree_nodes_.size(); ++id) {
    const NodeExtras *node_extras = static_cast<const NodeExtras*>(dom_tree_nodes_[id]->extras());
    char_num += node_extras->char_num;
    tag_num += node_extras->tag_num;
    link_char_num += node_extras->link_char_num;
    link_tag_num += node_extras->link_tag_num;
    punctuation_num += node_extras->punctuation_num;
    tag_h_num += node_extras->tag_h_num;
    tag_p_num += node_extras->tag_p_num;
  }
  nonlink_char_num = char_num - link_char_num;

  text_density = char_num / (tag_num + 1.0);
  punctuation_density = punctuation_num / (char_num + 1.0);
  tag_h_density = tag_h_num / (tag_num + 1.0);
  tag_p_density = tag_p_num / (tag_num + 1.0);

  link_char_density = link_char_num / (char_num + 1.0);
  link_tag_density = link_tag_num / (tag_num + 1.0);
}
void NodeCluster::CalcNonlinkCharBodyRatio(int32 nonlink_char_body_num) {
  nonlink_char_body_ratio = nonlink_char_num / (nonlink_char_body_num + 1.0);
}
void NodeCluster::CalcCharPerplexity() {
  std::set<int> char_set;
  std::string text;
  node_num = (int)dom_tree_nodes_.size();
  for (int id = 0; id < node_num; ++id) {
    if (dom_tree_nodes_[id]->TagType() == web::HTMLTag::kPureText) {
      const NodeExtras *p_node_extras = static_cast<const NodeExtras*>(dom_tree_nodes_[id]->extras());
      text += " " + p_node_extras->text;
    }
  }
  text_length = base::GetUTF8CharNumOrDie(text);
  base::UTF8CharIterator iter(&text);
  for (; !iter.end(); iter.Advance()) {
    int codepoint = iter.get();
    char_set.insert(codepoint);
  }
  distinct_char_num = (int)char_set.size();
  text_length_per_node = text_length / (node_num + 0.1);
  char_perplexity = distinct_char_num / (text_length + 1.0);
}
void NodeCluster::CalcNodePerplexity() {
  std::set<std::string> text_set;
  std::string text;
  node_num = (int)dom_tree_nodes_.size();
  for (int id = 0; id < node_num; ++id) {
    if (dom_tree_nodes_[id]->TagType() == web::HTMLTag::kPureText) {
      const NodeExtras *p_node_extras = static_cast<const NodeExtras*>(dom_tree_nodes_[id]->extras());
      text_set.insert(p_node_extras->text);
    }
  }
  distinct_node_num = (int)text_set.size();
  node_perplexity = distinct_node_num / (node_num + 1.0);
}
std::string NodeCluster::NodeClusterText() const {
  std::string text;
  for (int id = 0; id < (int)dom_tree_nodes_.size(); ++id) {
    if (dom_tree_nodes_[id]->TagType() == web::HTMLTag::kPureText) {
      const NodeExtras *p_node_extras = static_cast<const NodeExtras*>(dom_tree_nodes_[id]->extras());
      text += p_node_extras->text + "\n";
    }
  }
  return text;
}
}
