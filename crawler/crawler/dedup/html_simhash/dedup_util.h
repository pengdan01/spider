#pragma once

#include <vector>
#include <string>
#include <utility>

#include "base/common/basic_types.h"
namespace crawler {
// 分成 5 个 block
// Consider f = 64, k = 3, d = 34, pi = 25
const int kBlockNumber = 5;

// 置换函数, 将 |sign| 的 |block_id1|, |block_id2| 分别和 |sign| 第 0, 第 1 block 进行交换
uint64 Permute(uint64 sign, int block_id1, int block_id2);

// 构造 10 个表
void BuildTables(uint64 sign, std::vector<uint64> *sign_table);

// 截取 |sign| 的 前 |leading_bit_number| 位, 返回一个 uint64
uint64 TruncateLeadingN(uint64 sign, int leading_bit_number);
}  // end crawler
