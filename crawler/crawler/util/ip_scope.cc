#include "crawler/util/ip_scope.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <stack>
#include <fstream>
#include <algorithm>

#include "base/common/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_tokenize.h"

namespace crawler {

static bool is_lesser(const EdgeNode &f1, const EdgeNode &f2) {
  return f1.ip < f2.ip;
}

bool ConvertIp2Int(const std::string &ip_str, uint32_t &ip) {
  std::string my_str =  base::CollapseWhitespace(ip_str, true);
  if (my_str.empty()) {
    LOG(ERROR) << "call ConvertIp2Int() fail, input ip str is empty.";
    return false;
  }
  std::vector<std::string> tokens;
  size_t field_num = base::Tokenize(my_str, ".", &tokens);
  if (field_num != 4U || tokens[0].empty() ||
      tokens[1].empty() || tokens[2].empty() || tokens[3].empty()) {
    LOG(ERROR) << "input ip format error: " << my_str << ", should be dot seperated and 4 fields.";
    return false;
  }
  int32_t ip1 = atoi(tokens[0].c_str());
  int32_t ip2 = atoi(tokens[1].c_str());
  int32_t ip3 = atoi(tokens[2].c_str());
  int32_t ip4 = atoi(tokens[3].c_str());

  ip = (uint32_t)ip1 * (1 << 24) + (uint32_t)ip2 * (1<< 16) + (uint32_t)ip3 * (1<< 8) + (uint32_t)ip4;

  return true;
}

// Each record format like this:
// 1.1.2.0 - 1.1.3.255 : 1.1.2/23 : cn : 1.1.2 - 1.1.63 : APNIC
bool BuildIPScopeFromFile(const char*filename, std::vector<EdgeNode> *ip_scopes) {
  CHECK_NOTNULL(filename);

  std::ifstream fin(filename);
  LOG_IF(FATAL, !fin.good()) << "open file:" << filename << " fail.";

  std::string line;
  std::vector<std::string> tokens, ips;
  std::vector<EdgeNode> *tmp_ips = new std::vector<EdgeNode>();
  CHECK_NOTNULL(tmp_ips);

  ip_scopes->clear();
  size_t field_num;
  while (getline(fin, line)) {
    if (line.empty() || line[0] == '#') continue;
    tokens.clear();
    field_num = base::Tokenize(line, ":", &tokens);
    CHECK_EQ(field_num, 5U) << "File record format not right,should be 5 field seperated by ':'";

    if (base::CollapseWhitespace(tokens[2], true) != "cn") continue;
    ips.clear();
    field_num = base::Tokenize(tokens[0], "-", &ips);
    CHECK_EQ(field_num, 2U) << "parse 1st field whith seperator'-' fail";

    uint32_t ip1, ip2;
    LOG_IF(FATAL, !ConvertIp2Int(ips[0], ip1) || !ConvertIp2Int(ips[1], ip2)) << "call ConvertIp2Int() fail";

    EdgeNode node1, node2;
    node1.ip = ip1;
    node1.flag = 'b';
    node2.ip = ip2;
    node2.flag = 'e';
    tmp_ips->push_back(node1);
    tmp_ips->push_back(node2);
  }

  // sort
  if (tmp_ips->size() > 1) {
    stable_sort(tmp_ips->begin(), tmp_ips->end(), is_lesser);
    std::stack<EdgeNode> *tmp_stack = new std::stack<EdgeNode>();
    CHECK_NOTNULL(tmp_stack);
    std::vector<EdgeNode>::iterator it = tmp_ips->begin();
    tmp_stack->push(*it);
    ++it;
    while (it != tmp_ips->end()) {
      while (it != tmp_ips->end() && it->flag == tmp_stack->top().flag) {
        tmp_stack->push(*it);
        it++;
      }
      if (it == tmp_ips->end()) {
        break;
      } else {
        if (tmp_stack->size() == 1) {
          ip_scopes->push_back(tmp_stack->top());
          tmp_stack->pop();
          ip_scopes->push_back(*it);
          ++it;
          if (it != tmp_ips->end()) {
            tmp_stack->push(*it);
            ++it;
          }
        } else {
          tmp_stack->pop();
          ++it;
        }
      }
    }
    LOG_IF(FATAL, tmp_stack->empty() == false) << "stack not empty,and ip border dose not match.";
    delete tmp_stack;
  }
  delete tmp_ips;
  return true;
}

// 假设 ip 已经进行了大小排序
bool InCurrentIPScope(const std::vector<EdgeNode> *ip_scopes, uint32 ip) {
  CHECK_NOTNULL(ip_scopes);
  CHECK_GE(ip_scopes->size(), 2U);
  EdgeNode tmp_node;
  tmp_node.ip = ip;
  std::vector<EdgeNode>::const_iterator it = lower_bound(ip_scopes->begin(),
                                                           ip_scopes->end(), tmp_node);
  if (it == ip_scopes->end()) {
    return false;
  } else {
    if (it->ip != ip) {
      if (it != ip_scopes->begin()) {
        return (--it)->flag == 'b';
      } else {
        return false;
      }
    }
  }
  return true;
}

bool InCurrentIPScope(const std::vector<EdgeNode> *ip_scopes, const std::string &ip_str) {
  uint32 ip_num;
  if (!ConvertIp2Int(ip_str, ip_num)) {
    LOG(ERROR) << "Error,call ConvertIp2Int() fail, target ip: " << ip_str;
    return false;
  }
  return InCurrentIPScope(ip_scopes, ip_num);
}

/*
bool GetDomainFromUrl(const std::string &src_url, std::string &domain) {
  if (src_url.empty()) {
    LOG(WARNING) << "Input parameter src_url is empty";
    return false;
  }
  const std::string http_pattern("http://");
  bool ret = base::StartsWith(src_url, http_pattern, false);
  std::string src;
  if (ret == true) {
    src = src_url.substr(http_pattern.size());
  } else {
    src = src_url;
  }
  std::string::size_type pos = src.find_first_of("/?");
  if (pos == std::string::npos) {
    domain = src;
  } else {
    domain = src.substr(0, pos);
  }
  base::TrimTrailingWhitespaces(&domain);
  return true;
}
*/

bool GetIpFromDomain(const std::string &domain, std::vector<std::string> *ips) {
  if (domain.empty() || !ips) {
    LOG(ERROR) << "Input parameter |domain| is empty or |ips| is NULL.";
    return false;
  }
  struct hostent host, *phost;
  char dns_buffer[4096];
  int rc, ret;
  ret = gethostbyname_r(domain.c_str(), &host, dns_buffer, 4096, &phost, &rc);
  if (ret != 0 || phost == NULL) {
    LOG(ERROR) << "Threadid: " << pthread_self() << ", call gethostbyname_r() fail";
    return false;
  }
  ips->clear();
  for (int i = 0; phost->h_addr_list[i]; ++i) {
    const char *ip = inet_ntoa(*((struct in_addr *)phost->h_addr_list[i]));
    ips->push_back(std::string(ip));
  }
  if (ips->size() == 0) {
    return false;
  }
  return true;
}
/*
bool GetIpFromDomain(const std::string &domain, std::vector<std::string> *ips) {
  if (domain.empty() || !ips) {
    LOG(ERROR) << "Input parameter |domain| is empty or |ips| is NULL.";
    return false;
  }
  struct addrinfo *answer, hint, *curr;
  bzero(&hint, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  int ret = getaddrinfo(domain.c_str(), NULL, &hint, &answer);
  if (ret != 0 || answer == NULL) {
    PLOG(ERROR) << "call getaddrinfo() fail, error msg:" << gai_strerror(ret);
    return false;
  }

  ips->clear();
  char ipstr[16];
  for (curr = answer; curr != NULL; curr = curr->ai_next) {
    inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), ipstr, 16);
    ips->push_back(std::string(ipstr));
  }

  freeaddrinfo(answer);
  return true;
}
*/
bool IsDomainInCountry(const std::string &domain, const std::vector<EdgeNode> *ip_scopes) {
  if (domain.empty()) {
    return false;
  }
  CHECK_NOTNULL(ip_scopes);
  std::vector<std::string> ips;
  bool ret = GetIpFromDomain(domain, &ips);
  if (ret == false) {
    LOG(ERROR) << "Fail to get ip from domain: " << domain;
    return false;
  }
  for (size_t i = 0; i < ips.size(); ++i) {
    ret = InCurrentIPScope(ip_scopes, ips[i]);
    // 当该域名有一个 ip 不是国内 ip ，则认为该域名为国外域名，直接返回 false
    if (!ret) {
      return false;
    }
  }
  return true;
}

}  // end namespace
