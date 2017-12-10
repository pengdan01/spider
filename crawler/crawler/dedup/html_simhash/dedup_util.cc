#include "crawler/dedup/html_simhash/dedup_util.h"

#include <iostream>
#include <utility>
#include <bitset>

#include "base/strings/string_util.h"

namespace crawler {

// 从高位到地位
const int kBlocks[] = {13, 13, 13, 13, 12};
// 在 bitmap 中的起始 index
const int kBlocksStartIndex[] = {63, 50, 37, 24, 11, -1};

uint64 Permute(uint64 sign, int block_id1, int block_id2) {
  int block_numbers = arraysize(kBlocks);
  LOG_IF(FATAL, block_id1 >= block_numbers || block_id2 >= block_numbers)
      << "Invalid block id, should be [0, " << block_numbers << "]";
  std::bitset<64> bitmap;
  std::bitset<64> bitmap2(sign);
  int i = 63;
  int start_pos;
  for (start_pos = kBlocksStartIndex[block_id1]; start_pos > kBlocksStartIndex[block_id1 + 1]; --start_pos) {
    bitmap[i--] = bitmap2[start_pos];
  }
  for (start_pos = kBlocksStartIndex[block_id2]; start_pos > kBlocksStartIndex[block_id2 + 1]; --start_pos) {
    bitmap[i--] = bitmap2[start_pos];
  }
  for (int j = 0; j < block_numbers; ++j) {
    if (j == block_id1 || j == block_id2) continue;
    for (start_pos = kBlocksStartIndex[j]; start_pos > kBlocksStartIndex[j + 1]; --start_pos) {
      bitmap[i--] = bitmap2[start_pos];
    }
  }
  return bitmap.to_ulong();
}

void BuildTables(uint64 sign, std::vector<uint64> *sign_table) {
  CHECK_NOTNULL(sign_table);
  sign_table->clear();
  int block_numbers = arraysize(kBlocks);
  for (int i = 0; i < block_numbers; ++i) {
    for (int j = i+1; j < block_numbers; ++j) {
      uint64 newsign = Permute(sign, i, j);
      sign_table->push_back(newsign);
    }
  }
}

uint64 TruncateLeadingN(uint64 sign, int leading_bit_number) {
  CHECK(leading_bit_number > 0 && leading_bit_number <= 64);
  int right_move_bit = 64 - leading_bit_number;
  return (sign >> right_move_bit);
}
}  // end crawler
