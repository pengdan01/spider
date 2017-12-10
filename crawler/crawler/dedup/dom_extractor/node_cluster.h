#pragma once

#include <vector>
#include <set>
#include <string>

#include "base/common/base.h"
#include "web/html/api/parser.h"
#include "crawler/dedup/dom_extractor/node_extras.h"

namespace dom_extractor {
struct NodeCluster {
  NodeCluster();
  ~NodeCluster();
  void Clear();
  void CalcClusterFeature();
  void CalcNonlinkCharBodyRatio(int32 nonlink_char_body_num);
  void CalcCharPerplexity();
  void CalcNodePerplexity();
  std::string NodeClusterText() const;

  std::vector<web::HTMLNode*> dom_tree_nodes_;
  std::string path_;
  int32 indent_;

  NodeCluster *parent_;
  NodeCluster *first_child_;
  NodeCluster *next_sibling_;


  bool is_main_content;

  int char_num;
  int tag_num;
  int link_char_num;
  int link_tag_num;
  int punctuation_num;
  int tag_h_num;
  int tag_p_num;

  int nonlink_char_num;               // = char_num - link_char_num;
  double nonlink_char_body_ratio;     // = nonlink_char_num / body.nonlink_char_num;
  double text_density;                // = char_num / tag_num,  positive weight
  double punctuation_density;         // = punctuation / char_num, positive weight
  double tag_h_density;               // = tag_p_num / tag_num, positive weight
  double tag_p_density;               // = tag_h_num / tag_num, positive weight

  double link_char_density;           // = link_char_num / char_num, negative weight
  double link_tag_density;            // = link_tag_num / tag_num, negative weight

  int node_num;
  int text_length;
  int distinct_char_num;
  int distinct_node_num;
  float text_length_per_node;
  float char_perplexity;
  float node_perplexity;
};
}
