#include "crawler/dedup/dom_extractor/content_collector.h"

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <map>

#include "base/common/base.h"
#include "base/common/slice.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_char_iterator.h"
#include "nlp/common/nlp_util.h"
#include "base/strings/utf_string_conversions.h"
#include "web/html/api/parser.h"
#include "crawler/dedup/dom_extractor/node_extras.h"
#include "crawler/dedup/dom_extractor/node_cluster.h"

DEFINE_int32(max_node_num, 50000, "the maximum number of nodes");
DEFINE_int32(max_depth, 1000, "maximum depth of the tree");
DEFINE_int32(max_node_cluster_num_, 5000, "the maximum number of node clusters");
DEFINE_int32(indent, 20, "dump indent");
DEFINE_double(body_ratio_thresh, 0.5, "body ratio thresh");

namespace dom_extractor {
ContentCollector::ContentCollector() {
  max_node_num_ = FLAGS_max_node_num;
  max_depth_ = FLAGS_max_depth;
  max_node_cluster_num_ = FLAGS_max_node_cluster_num_;

  node_num_ = 0;
  depth_ = 0;
  node_cluster_num_ = 0;

  node_extras_ = new NodeExtras[max_node_num_];
  node_clusters_ = new NodeCluster[max_node_cluster_num_];
  path_clusters_.resize(max_depth_);

  body_ = NULL;
  head_ = NULL;
}
ContentCollector::~ContentCollector() {
  delete []node_extras_;
  delete []node_clusters_;
  body_ = NULL;
  head_ = NULL;
}
void ContentCollector::Clear() {
  node_cluster_num_ = 0;
  node_num_ = 0;
  depth_ = 0;
  body_ = NULL;
  head_ = NULL;

  for (int id = 0; id < max_node_cluster_num_; ++id) {
    node_clusters_[id].Clear();
  }

  for (int id = 0; id < max_node_num_; ++id) {
    node_extras_[id].Clear();
  }

  for (int id = 0; id < max_depth_; ++id) {
    path_clusters_[id].clear();
  }
}
bool ContentCollector::ExtractMainContent(
    const std::string &page_utf8,
    const std::string &url,
    std::string *title,
    std::string *content) {

  // LOG(ERROR) << "begin maincontent";
  Clear();
  title->clear();
  content->clear();
  // LOG(ERROR) << "Clear ok";

  web::HTMLTree tree;
  web::HTMLParser parser;
  uint32 parse_mode = web::HTMLParser::kParseInternalCSS;
  // if (parser_.ParseString(page_utf8.c_str(), parse_mode, &tree_) == web::HTMLParser::kParserFail) {
  if (parser.ParseString(page_utf8.c_str(), parse_mode, &tree) != 0u) {
    LOG(ERROR) << "Parse tree from string failed:\t" << url;
    return false;
  }

  // LOG(ERROR) << "ParseString ok";

  head_ = tree.head();
  if (head_ == NULL) {
    LOG(ERROR) << "NULL head:\t" << url;
    return false;
  }
  std::string raw_title = tree.title_value().as_string();
  // base::TrimString(raw_title, "\n\t\r ", title);
  *title = raw_title;

  body_ = tree.body();
  if (body_ == NULL) {
    LOG(ERROR) << "NULL body:\t" << url;
    return false;
  }

  if (!SetNodeExtras(body_)) {
    LOG(ERROR) << "SetNodeExtras failed for url:\t" << url;
    return false;
  }

  if (!RemoveNontextTopDown(body_)) {
    LOG(ERROR) << "RemoveNontextTopDown failed:\t" << url;
    return false;
  }

  if (!RemoveNontextBottomUp(body_)) {
    LOG(ERROR) << "Body has no text:\t" << url;
    return true;
  }

  // DumpTree(*body_);

  if (!ConstructNodeCluster(body_)) {
    LOG(ERROR) << "ConstructNodeCluster failed for url:\t" << url;
    return false;
  }

  if (!ConstructNodeClusterTree()) {
    LOG(ERROR) << "ConstructNodeClusterTree failed for url:\t" << url;
    return false;
  }

  // DumpNodeClusterTree(node_clusters_[0]);

  // DumpTree(*body_);
  for (int id = 0; id < node_cluster_num_; ++id) {
    if (node_clusters_[id].first_child_ == NULL) {
      node_clusters_[id].CalcNodePerplexity();
      node_clusters_[id].CalcCharPerplexity();
    }
  }

  RemoveByCharPerplexity();

  if (!CollectNodeStatistic(body_)) {
    LOG(ERROR) << "CollectNodeStatistic failed for url:\t" << url;
    return false;
  }

  if (!UpdateBodyRatio(body_)) {
    LOG(ERROR) << "UpdateBodyRatio failed for url:\t" << url;
    return false;
  }


  for (int id = 0; id < node_cluster_num_; ++id) {
    node_clusters_[id].CalcClusterFeature();
    node_clusters_[id].CalcNonlinkCharBodyRatio(node_clusters_[0].nonlink_char_num);
  }

  // DumpNodeClusterTree(node_clusters_[0]);

  is_identified = false;
  if (!FindDominantNodeCluster2(node_clusters_ + 0)) {
    LOG(ERROR) << "FindDominantNodeCluster failed" << url;
    return false;
  }
  if (!CollectMainContent(*body_, content)) {
    LOG(ERROR) << "CollectMainContent failed for url:\t" << url;
    return false;
  }

  /*
  if (!CollectMainContent(*(node_clusters_ + 0), content)) {
    LOG(ERROR) << "CollectMainContent failed for url:\t" << url;
    return false;
  }
  */
  /*
  if (!LargestNodeClusterText(*(node_clusters_ + 0), content)) {
    LOG(ERROR) << "LargestNodeClusterText failed for url:\t" << url;
    return false;
  }
  */
  nlp::util::NormalizeLineInPlaceS(content);
  return true;
}
void ContentCollector::DumpNodeClusterTree(const NodeCluster &node_cluster) {
  NodeCluster *child = node_cluster.first_child_;
  if (node_cluster.is_main_content && !child && node_cluster.nonlink_char_num) {
    std::cout << node_cluster.path_
              << "\t" << node_cluster.is_main_content
              // << "\t" << node_cluster.nonlink_char_body_ratio
              << "\t" << node_cluster.nonlink_char_num
              // << "\t" << node_cluster.dom_tree_nodes_.size()
              // << "\t" << node_cluster.distinct_node_num
              << "\t" << node_cluster.node_perplexity
              // << "\t" << node_cluster.distinct_char_num
              << "\t" << node_cluster.char_perplexity
              << std::endl;
    if (!child) {
      std::cout << node_cluster.NodeClusterText();
    }
  }
  if (child) {
    while (child) {
      DumpNodeClusterTree(*child);
      child = child->next_sibling_;
    }
  }
}
void ContentCollector::DumpTree(const web::HTMLNode &node) {
  const NodeExtras *p_node_extras = static_cast<const NodeExtras*>(node.extras());
  if (p_node_extras->has_text
      // && node.first_child() == NULL
      && p_node_extras->indent < FLAGS_indent
      ) {
    std::cout << p_node_extras->node_id.path;
    std::cout
        // << "\t" << p_node_extras->nonlink_char_body_ratio
        // << "\t" << p_node_extras->nonlink_char_num
        // << "\t" << p_node_extras->link_char_num
        // << "\t" << p_node_extras->punctuation_num
        << "\t" << p_node_extras->text;
    std::cout << std::endl;
  }
  web::HTMLNode *child = node.first_child();
  while (child) {
    DumpTree(*child);
    child = child->next_sibling();
  }
}
bool ContentCollector::SetNodeExtras(web::HTMLNode *node) {
  web::HTMLNode *parent  = node->parent();
  const NodeExtras *p_parent_extras = static_cast<const NodeExtras*>(parent->extras());
  NodeExtras *p_node_extras = node_extras_ + node_num_;

  p_node_extras->node_id.tag_type = node->TagType();
  p_node_extras->node_id.tag_name = node->TagName();
  p_node_extras->node_id.id_name = node->AttrValue(web::HTMLAttr::AttrType::kId);
  p_node_extras->node_id.class_name = node->AttrValue(web::HTMLAttr::AttrType::kClass);
  // p_node_extras->node_id.full_name = p_node_extras->node_id.tag_name;
  p_node_extras->node_id.full_name = p_node_extras->node_id.tag_name
      + "_" + p_node_extras->node_id.class_name;

  if (p_parent_extras == NULL) {
    p_node_extras->indent = 0;
    p_node_extras->node_id.path = p_node_extras->node_id.full_name;
  } else {
    p_node_extras->indent = p_parent_extras->indent + 1;
    p_node_extras->node_id.path = p_parent_extras->node_id.path + "/" + p_node_extras->node_id.full_name;
  }
  if (p_node_extras->indent + 1 > depth_) {
    depth_ = p_node_extras->indent + 1;
  }
  if (depth_ >= max_depth_) {
    LOG(ERROR) << "Tree depth exceeds " << "\t" << depth_ << "\t" << max_depth_;
    return false;
  }
  node->set_extras(static_cast<void*>(p_node_extras));
  node_num_++;
  if (node_num_ >= max_node_num_) {
    LOG(ERROR) << "Node number exceeds " << "\t" << node_num_ << "\t" << max_node_num_;
    return false;
  }
  web::HTMLNode *child = node->first_child();
  while (child) {
    if (!SetNodeExtras(child)) {
      return false;
    }
    child = child->next_sibling();
  }
  return true;
}
bool ContentCollector::RemoveNontextTopDown(web::HTMLNode *node) {
  web::HTMLNode *parent  = node->parent();
  const NodeExtras *p_parent_extras = static_cast<const NodeExtras*>(parent->extras());
  NodeExtras *p_node_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node->extras()));
  if (p_parent_extras == NULL) {
    p_node_extras->has_text = true;
  } else {
    if (!p_parent_extras->has_text) {
      p_node_extras->has_text = false;
    } else {
      if (DroppableNode(*node)) {
        p_node_extras->has_text = false;
      } else {
        p_node_extras->has_text = true;
      }
    }
  }
  web::HTMLNode *child = node->first_child();
  while (child) {
    if (!RemoveNontextTopDown(child)) {
      return false;
    }
    child = child->next_sibling();
  }
  return true;
}
bool ContentCollector::RemoveNontextBottomUp(web::HTMLNode *node) {
  NodeExtras *p_node_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node->extras()));
  if (!p_node_extras->has_text) {
    return false;
  }
  web::HTMLNode *child = node->first_child();
  if (child) {
    bool has_valid_child = false;
    while (child) {
      if (RemoveNontextBottomUp(child)) {
        has_valid_child = true;
      }
      child = child->next_sibling();
    }
    if (has_valid_child) {
      p_node_extras->has_text = true;
    } else {
      p_node_extras->has_text = false;
    }
  } else {
    if (node->TagType() == web::HTMLTag::kPureText) {
      std::string text = node->value().as_string();
      if (!TextStatistic(text, p_node_extras)) {
        LOG(ERROR) << "TextStatistic failed:\t" << text;
        return false;
      }
      if (p_node_extras->char_num != 0) {
        p_node_extras->has_text = true;
      } else {
        p_node_extras->has_text = false;
      }
    } else {
      p_node_extras->has_text = false;
    }
  }
  return p_node_extras->has_text;
}
bool ContentCollector::CollectNodeStatistic(web::HTMLNode *node) {
  NodeExtras *p_node_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node->extras()));
  if (p_node_extras == NULL) {
    LOG(ERROR) << "NULL node extras";
    return false;
  }
  if (node->first_child()) {
    p_node_extras->SetCounterZero();
    web::HTMLNode *child = node->first_child();
    while (child) {
      if (!CollectNodeStatistic(child)) {
        return false;
      }
      NodeExtras *p_child_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(child->extras()));
      CHECK_NOTNULL(p_child_extras);
      if (p_child_extras->has_text) {
        p_node_extras->AddChild(*p_child_extras);
      }
      child = child->next_sibling();
    }
    if (!CollectNonleafStatistic(node)) {
      return false;
    }
  } else {
    if (!CollectLeafStatistic(node)) {
      return false;
    }
  }
  return true;
}
bool ContentCollector::UpdateBodyRatio(web::HTMLNode *node) {
    const NodeExtras *p_body_extras = static_cast<const NodeExtras*>(body_->extras());
    NodeExtras *p_node_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node->extras()));
    p_node_extras->nonlink_char_body_ratio
        = p_node_extras->nonlink_char_num / (p_body_extras->nonlink_char_num + 1.0);

    if (node->first_child()) {
      web::HTMLNode *child = node->first_child();
      while (child) {
        if (!UpdateBodyRatio(child)) {
          return false;
        }
        child = child->next_sibling();
      }
    }
    return true;
}
bool ContentCollector::ConstructNodeCluster(web::HTMLNode *node) {
  const NodeExtras *p_node_extras = static_cast<const NodeExtras*>(node->extras());
  if (p_node_extras->has_text) {
    int indent = p_node_extras->indent;
    std::map<std::string, int32>::iterator path_cluster_it
        = path_clusters_[indent].find(p_node_extras->node_id.path);
    if (path_cluster_it == path_clusters_[indent].end()) {
      // 新发现路径
      path_clusters_[indent].insert(std::make_pair(p_node_extras->node_id.path, node_cluster_num_));
      node_clusters_[node_cluster_num_].dom_tree_nodes_.push_back(node);
      node_clusters_[node_cluster_num_].path_ = p_node_extras->node_id.path;
      node_cluster_num_++;
      if (node_cluster_num_ >= max_node_cluster_num_) {
        LOG(ERROR) << "node_cluster_num_ >= max_node_cluster_num_";
        return false;
      }
    } else {
      node_clusters_[path_cluster_it->second].dom_tree_nodes_.push_back(node);
    }
  }
  web::HTMLNode *child = node->first_child();
  while (child) {
    NodeExtras *p_child_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(child->extras()));
    if (p_child_extras->has_text) {
      if (!ConstructNodeCluster(child)) {
        return false;
      }
    }
    child = child->next_sibling();
  }
  return true;
}
bool ContentCollector::ConstructNodeClusterTree() {
  // id = 0 是根节点
  node_clusters_[0].indent_ = 0;
  for (int id = 1; id < node_cluster_num_; ++id) {
    web::HTMLNode *node = node_clusters_[id].dom_tree_nodes_[0];
    const NodeExtras *node_extras = static_cast<const NodeExtras*>(node->extras());
    std::string node_path = node_extras->node_id.path;
    int32 node_indent = node_extras->indent;
    std::string parent_path = node_path.substr(0, node_path.find_last_of('/'));
    std::map<std::string, int32>::iterator path_cluster_it
        = path_clusters_[node_indent - 1].find(parent_path);
    if (path_cluster_it == path_clusters_[node_indent - 1].end()) {
      LOG(ERROR) << "Can not find parent node cluster";
      return false;
    }
    int parent_node_cluster = path_cluster_it->second;
    NodeCluster *parent = node_clusters_ + parent_node_cluster;

    node_clusters_[id].parent_ = parent;
    node_clusters_[id].indent_ = parent->indent_ + 1;
    NodeCluster *child = parent->first_child_;
    if (child == NULL) {
      parent->first_child_ = node_clusters_ + id;
    } else {
      while (child->next_sibling_) {
        child = child->next_sibling_;
      }
      child->next_sibling_ = node_clusters_ + id;
    }
  }
  return true;
}
bool ContentCollector::RemoveByCharPerplexity() {
  for (int id = 0; id < node_cluster_num_; ++id) {
    // if (node_clusters_[id].first_child_ == NULL && node_clusters_[id].is_main_content) {
    if (node_clusters_[id].first_child_ == NULL) {
      if (node_clusters_[id].node_perplexity < 0.2
          || (node_clusters_[id].distinct_char_num < 50 && node_clusters_[id].char_perplexity < 0.2)) {
        // node_clusters_[id].is_main_content = false;
        for (int i = 0; i < (int)node_clusters_[id].dom_tree_nodes_.size(); ++i) {
          NodeExtras *node_extras
              = const_cast<NodeExtras*>
              (static_cast<const NodeExtras*>(node_clusters_[id].dom_tree_nodes_[i]->extras()));
          node_extras->has_text = false;
        }
        /*
        std::cout << "perplexity rule:\t" << node_clusters_[id].path_
                  << "\t" << node_clusters_[id].distinct_node_num
                  << "\t" << node_clusters_[id].node_perplexity
                  << "\t" << node_clusters_[id].distinct_char_num
                  << "\t" << node_clusters_[id].text_length
                  << "\t" << node_clusters_[id].char_perplexity
                  << std::endl;
        std::cout << node_clusters_[id].NodeClusterText();
        */
      }
    }
  }
  return true;
}
bool ContentCollector::SetNodeMainContent(const bool &is_main_content_in, NodeCluster *node_cluster) {
  node_cluster->is_main_content = is_main_content_in;
  for (int id = 0; id < (int)node_cluster->dom_tree_nodes_.size(); ++id) {
    NodeExtras *node_extras
       = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node_cluster->dom_tree_nodes_[id]->extras()));
    node_extras->is_main_content = node_cluster->is_main_content;
  }
  return true;
}
bool ContentCollector::SetMainContent(const bool &is_main_content_in, NodeCluster *node_cluster) {
  if (!SetNodeMainContent(is_main_content_in, node_cluster)) {
    return false;
  }
  NodeCluster *child = node_cluster->first_child_;
  while (child) {
    if (!SetMainContent(is_main_content_in, child)) {
      return false;
    }
    child = child->next_sibling_;
  }
  return true;
}
bool ContentCollector::FindDominantNodeCluster(NodeCluster *node_cluster) {
  node_cluster->is_main_content = true;
  SetNodeMainContent(true, node_cluster);
  NodeCluster *child = node_cluster->first_child_;
  if (child) {
    double nonlink_char_body_ratio_sum = 0;
    double best_nonlink_char_body_ratio = 0;
    NodeCluster *best_child = NULL;
    while (child) {
      nonlink_char_body_ratio_sum += child->nonlink_char_body_ratio;
      if (child->nonlink_char_body_ratio > best_nonlink_char_body_ratio) {
        best_nonlink_char_body_ratio = child->nonlink_char_body_ratio;
        best_child = child;
      }
      child = child->next_sibling_;
    }
    if (nonlink_char_body_ratio_sum > 0 &&
        best_nonlink_char_body_ratio / nonlink_char_body_ratio_sum > FLAGS_body_ratio_thresh) {
      // 发现某分支占优
      NodeCluster *child = node_cluster->first_child_;
      while (child) {
        if (child == best_child) {
          // DLOG(ERROR) << "dominant:\t" << child->nonlink_char_body_ratio << "\t" << child->path_;
          /*
          std::cout << "dominant:\t" << child->nonlink_char_body_ratio << "\t" << child->path_;
          std::cout << "\t" << child->NodeClusterText();
          std::cout <<std::endl;
          */
          if (!FindDominantNodeCluster(child)) {
            return false;
          }
        } else {
          if (!SetMainContent(false, child)) {
            return false;
          }
          // DLOG(ERROR) << "delete:\t" << child->nonlink_char_body_ratio << "\t" << child->path_;
          /*
          std::cout << "delete:\t" << child->nonlink_char_body_ratio << "\t" << child->path_;
          std::cout << "\t" << child->NodeClusterText();
          std::cout << std::endl;
          */
        }
        child = child->next_sibling_;
      }
    } else {
      // 不存在某分支占优
      // DLOG(ERROR) << "stop finding dominant";
      NodeCluster *child = node_cluster->first_child_;
      while (child) {
        if (!SetMainContent(true, child)) {
          return false;
        }
        child = child->next_sibling_;
      }
    }
  }
  return true;
}
bool ContentCollector::FindDominantNodeCluster2(NodeCluster *node_cluster) {
  node_cluster->is_main_content = true;
  SetNodeMainContent(true, node_cluster);
  NodeCluster *child = node_cluster->first_child_;
  if (child) {
    double best_nonlink_char_body_ratio = 0;
    NodeCluster *best_child = NULL;
    while (child) {
      if (child->nonlink_char_body_ratio > best_nonlink_char_body_ratio) {
        best_nonlink_char_body_ratio = child->nonlink_char_body_ratio;
        best_child = child;
      }
      child = child->next_sibling_;
    }
    if (best_nonlink_char_body_ratio > FLAGS_body_ratio_thresh) {
      // 发现某分支占优
      NodeCluster *child = node_cluster->first_child_;
      while (child) {
        if (child == best_child) {
          if (!FindDominantNodeCluster(child)) {
            return false;
          }
        } else {
          if (!SetMainContent(false, child)) {
            return false;
          }
        }
        child = child->next_sibling_;
      }
    } else {
      // 不存在某分支占优
      NodeCluster *child = node_cluster->first_child_;
      while (child) {
        if (!SetMainContent(true, child)) {
          return false;
        }
        child = child->next_sibling_;
      }
    }
  }
  return true;
}
bool ContentCollector::CollectNonleafStatistic(web::HTMLNode *node) {
  NodeExtras *p_node_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node->extras()));
  if (CountTag(*node)) {
    p_node_extras->tag_num++;
  }
  if (node->TagType() == web::HTMLTag::kP) {
    p_node_extras->tag_p_num++;
  }
  if (node->TagType() == web::HTMLTag::kH1
      || node->TagType() == web::HTMLTag::kH2
      || node->TagType() == web::HTMLTag::kH3
      || node->TagType() == web::HTMLTag::kH4
      || node->TagType() == web::HTMLTag::kH5
      || node->TagType() == web::HTMLTag::kH6) {
    p_node_extras->tag_h_num++;
  }
  // if the node is a hyperlink, update the link_tag_num and link_char_num
  if (node->TagType() == web::HTMLTag::kA && p_node_extras->has_text) {
    p_node_extras->link_tag_num++;
    // NOTE(yuanjinhui): the pure text of its children should be counted as the link char
    p_node_extras->link_char_num += p_node_extras->char_num;
  }
  p_node_extras->nonlink_char_num = p_node_extras->char_num - p_node_extras->link_char_num;
  p_node_extras->UpdateDensity();
  return true;
}
bool ContentCollector::TextStatistic(const std::string &text, NodeExtras *node_extras) {
  CHECK_NOTNULL(node_extras);
  base::UTF8CharIterator iter(&text);
  node_extras->text = "";
  std::string utf_char = "";
  wchar_t wchar;
  for (; !iter.end(); iter.Advance()) {
    int codepoint = iter.get();
    // NODE(yuanjinhui): 不包括阿拉伯数字, 可能会丢失一些数字信息
    if ((codepoint >= 0x4E00 && codepoint <= 0x9FBF) ||  // All Chinese characters.
        (codepoint >= 0x0041 && codepoint <= 0x005A) ||  // All uppercase English characters.
        (codepoint >= 0x0061 && codepoint <= 0x007A) ||
        (codepoint >= 0xFF21 && codepoint <= 0xFF3A) ||
        (codepoint >= 0xFF41 && codepoint <= 0xFF5A)) {  // All lowercase English characters.
      node_extras->char_num++;
      wchar = (wchar_t)codepoint;
      base::WideToUTF8(&wchar, 1, &utf_char);
      node_extras->text.append(utf_char);
    } else if ((codepoint >= 0x0021 && codepoint <= 0x002F) ||
               (codepoint >= 0x003A && codepoint <= 0x0040) ||
               (codepoint >= 0x007B && codepoint <= 0x007E) ||
               (codepoint >= 0x2010 && codepoint <= 0x2027) ||
               (codepoint >= 0x3001 && codepoint <= 0x303F) ||
               (codepoint >= 0xFF01 && codepoint <= 0xFF0F) ||
               (codepoint >= 0xFF1A && codepoint <= 0xFF20) ||
               (codepoint >= 0xFF3B && codepoint <= 0xFF40) ||
               (codepoint >= 0xFF5B && codepoint <= 0xFF65) ||
               (codepoint >= 0xFFDA && codepoint <= 0xFFEE)) {
      node_extras->punctuation_num++;
      wchar = (wchar_t)codepoint;
      base::WideToUTF8(&wchar, 1, &utf_char);
      node_extras->text.append(utf_char);
    }
  }
  return true;
}
bool ContentCollector::CollectLeafStatistic(web::HTMLNode *node) {
  NodeExtras *p_node_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(node->extras()));

  if (node->TagType() == web::HTMLTag::kPureText) {
    // pure text leaf node
    if (!p_node_extras->has_text) {
      // 在 RemoveNontextBottomUp 函数已处理
      p_node_extras->SetCounterZero();
    }
  } else {
    // non-text leaf node, abnormal
    p_node_extras->SetCounterZero();
    if (CountTag(*node)) {
      p_node_extras->tag_num = 1;
    }
    if (node->TagType() == web::HTMLTag::kA) {
      p_node_extras->link_tag_num = 1;
    }
    if (node->TagType() == web::HTMLTag::kP) {
      p_node_extras->tag_p_num = 1;
    }
    if (node->TagType() == web::HTMLTag::kH1
        || node->TagType() == web::HTMLTag::kH2
        || node->TagType() == web::HTMLTag::kH3
        || node->TagType() == web::HTMLTag::kH4
        || node->TagType() == web::HTMLTag::kH5
        || node->TagType() == web::HTMLTag::kH6) {
      p_node_extras->tag_h_num = 1;
    }
  }
  p_node_extras->nonlink_char_num = p_node_extras->char_num - p_node_extras->link_char_num;
  p_node_extras->UpdateDensity();
  return true;
}
bool ContentCollector::CollectMainContent(const web::HTMLNode &node, std::string *content) {
  web::HTMLNode *child = node.first_child();
  if (child) {
    while (child) {
      NodeExtras *p_child_extras = const_cast<NodeExtras*>(static_cast<const NodeExtras*>(child->extras()));
      if (p_child_extras->is_main_content) {
        std::string child_text;
        if (!CollectMainContent(*child, &child_text)) {
          return false;
        }
        content->append(" " + child_text);
      }
      child = child->next_sibling();
    }
  } else {
    const NodeExtras *p_node_extras = static_cast<const NodeExtras*>(node.extras());
    if (p_node_extras->has_text &&
        p_node_extras->is_main_content &&
        node.TagType() == web::HTMLTag::kPureText) {
      std::string node_text = " " + p_node_extras->text;
      content->append(node_text);
    }
  }
  return true;
}
bool ContentCollector::LargestNodeClusterText(const NodeCluster &node_cluster, std::string *content) {
  NodeCluster *child = node_cluster.first_child_;
  if (node_cluster.is_main_content && !child && node_cluster.nonlink_char_num) {
    std::string node_text = node_cluster.NodeClusterText();
    if (node_text.length() > content->length()) {
      *content = node_text;
    }
  }
  if (child) {
    while (child) {
      LargestNodeClusterText(*child, content);
      child = child->next_sibling_;
    }
  }
  return true;
}
bool ContentCollector::CollectMainContent(const NodeCluster &node_cluster, std::string *content) {
  NodeCluster *child = node_cluster.first_child_;
  if (child) {
    while (child) {
      if (child->is_main_content) {
        std::string child_text;
        if (!CollectMainContent(*child, &child_text)) {
          return false;
        }
        content->append(" " + child_text);
      }
      child = child->next_sibling_;
    }
  } else {
    if (node_cluster.is_main_content) {
      content->append(node_cluster.NodeClusterText());
    }
  }
  return true;
}
bool ContentCollector::CountTag(const web::HTMLNode &node) {
  if (node.TagType() == web::HTMLTag::kPureText
      || node.TagType() == web::HTMLTag::kUnknown
      || node.TagType() == web::HTMLTag::kScript
      || node.TagType() == web::HTMLTag::kComment
      || node.TagType() == web::HTMLTag::kComment2
      || node.TagType() == web::HTMLTag::kNoscript
      || node.TagType() == web::HTMLTag::kIframe) {
    return false;
  } else {
    return true;
  }
}
bool ContentCollector::EmptyPureTextNode(const web::HTMLNode &node) {
  if (node.TagType() == web::HTMLTag::kPureText) {
    std::string text = node.value().as_string();
    std::string trim_text;
    base::TrimString(text, "\t\n\r ", &trim_text);
    if (!trim_text.empty()) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}
bool ContentCollector::UnVisibleNode(const web::HTMLNode &node) {
  const web::HTMLStyle *style = node.html_style();
  // special rule for baidu baike
  if (node.AttrValue(web::HTMLAttr::AttrType::kClass) == "polysemy-item-cnt clearfix") {
    return false;
  }
  // special rule for gome
  /*
  std::string id_name = node.AttrValue(web::HTMLAttr::AttrType::kId);
  if (node.AttrValue(web::HTMLAttr::AttrType::kClass) == "tabBox"
      && base::StartsWith(id_name, "tab_c", true)
      && !style->visible()) {
    std::cout << node.TagName() << "\t" << id_name << std::endl;
    return false;
  }
  */
  if (style == NULL) {
    return false;
  } else {
    return !style->visible();
  }
}
bool ContentCollector::DroppableNode(const web::HTMLNode &node) {
  return
      node.TagType() == web::HTMLTag::kComment
      || node.TagType() == web::HTMLTag::kComment2

      || node.TagType() == web::HTMLTag::kHr
      || node.TagType() == web::HTMLTag::kBr

      // control
      || node.TagType() == web::HTMLTag::kInput
      || node.TagType() == web::HTMLTag::kSelect
      || node.TagType() == web::HTMLTag::kTextarea
      || node.TagType() == web::HTMLTag::kMenu
      || node.TagType() == web::HTMLTag::kApplet
      || node.TagType() == web::HTMLTag::kButton
      || node.TagType() == web::HTMLTag::kObject

      || node.TagType() == web::HTMLTag::kNav
      || node.TagType() == web::HTMLTag::kFooter
      || node.TagType() == web::HTMLTag::kHeader

      || node.TagType() == web::HTMLTag::kIframe

      || node.TagType() == web::HTMLTag::kSound
      || node.TagType() == web::HTMLTag::kDatagrid
      || node.TagType() == web::HTMLTag::kLink
      || node.TagType() == web::HTMLTag::kImg
      || node.TagType() == web::HTMLTag::kStyle
      || node.TagType() == web::HTMLTag::kScript
      || node.TagType() == web::HTMLTag::kNoscript
      || node.TagType() == web::HTMLTag::kMarquee

      || node.TagType() == web::HTMLTag::kA
      || EmptyPureTextNode(node)
      || UnVisibleNode(node);
}
}
