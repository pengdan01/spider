#pragma once
#include <map>
#include <string>
#include <vector>
// #include "web/html/api/parser.h"
#include "crawler/dedup/dom_extractor/node_cluster.h"
#include "crawler/dedup/dom_extractor/node_extras.h"

namespace dom_extractor {

class ContentCollector {
 public:
  ContentCollector();
  ~ContentCollector();
  // 提取网页 |page_utf8| 中的标题 |title| 和正文 |content|, 输入参数 |url| 可以为空串
  // 成功返回 true,  反之返回 false
  bool ExtractMainContent(const std::string &page_utf8,
                          const std::string &url,
                          std::string *title,
                          std::string *content);

 private:
  void Clear();

  bool SetNodeExtras(web::HTMLNode *node);
  bool RemoveNontextTopDown(web::HTMLNode *node);
  bool RemoveNontextBottomUp(web::HTMLNode *node);
  bool CollectNodeStatistic(web::HTMLNode *node);
  bool UpdateBodyRatio(web::HTMLNode *node);
  bool CollectMainContent(const web::HTMLNode &node, std::string *content);
  bool CollectMainContent(const NodeCluster &node_cluster, std::string *content);
  void DumpTree(const web::HTMLNode &node);
  bool ConstructNodeCluster(web::HTMLNode *node);
  bool ConstructNodeClusterTree();
  void DumpNodeClusterTree(const NodeCluster &node_cluster);
  bool FindDominantNodeCluster(NodeCluster *node_cluster);
  bool FindDominantNodeCluster2(NodeCluster *node_cluster);
  bool LargestNodeClusterText(const NodeCluster &node_cluster, std::string *content);

  bool TextStatistic(const std::string &text, NodeExtras *node_extras);
  bool EmptyPureTextNode(const web::HTMLNode &node);
  bool UnVisibleNode(const web::HTMLNode &node);
  bool DroppableNode(const web::HTMLNode &node);
  bool CountTag(const web::HTMLNode &node);
  bool CollectNonleafStatistic(web::HTMLNode *node);
  bool CollectLeafStatistic(web::HTMLNode *node);
  bool SetNodeMainContent(const bool &is_main_content_in, NodeCluster *node_cluster);
  bool SetMainContent(const bool &is_main_content_in, NodeCluster *node_cluster);
  bool RemoveByCharPerplexity();

  bool is_identified;

  int32 max_node_num_;
  int32 node_num_;
  int32 max_depth_;
  int32 depth_;
  int32 max_node_cluster_num_;
  int32 node_cluster_num_;

  // web::HTMLTree tree_;
  // web::HTMLParser parser_;
  NodeExtras *node_extras_;
  NodeCluster *node_clusters_;
  std::vector<std::map<std::string, int32> > path_clusters_;
  web::HTMLNode *body_;
  web::HTMLNode *head_;
};
}
