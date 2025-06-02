#ifndef UTILS_BLOCK_H_
#define UTILS_BLOCK_H_

#include <cryptoTools/Common/block.h>

namespace fsswm {

using block = osuCrypto::block;

/**
 * @brief Create a block from two 64-bit integers.
 * @param high The higher 64-bit integer.
 * @param low The lower 64-bit integer.
 * @return The block created from the two integers.
 */
inline block MakeBlock(uint64_t high, uint64_t low) {
    return osuCrypto::toBlock(high, low);
}

/**
 * @brief Get the least significant bit of a block.
 * @param x The block to be checked.
 * @return The least significant bit of the block.
 */
inline bool GetLsb(const block &x) {
    // Extract low 64 bits then check bit-0
    return (_mm_extract_epi64(x.mData, 0) & 1) != 0;
}

/**
 * @brief Set the least significant bit of a block to zero.
 * @param x The block to be modified.
 */
inline void SetLsbZero(block &x) {
    // Set the least significant bit to zero
    x.mData = _mm_andnot_si128(_mm_set_epi64x(0, 1), x.mData);
}

// Predefined constants
static const block                zero_block         = MakeBlock(0, 0);
static const block                one_block          = MakeBlock(0, 1);
static const block                not_one_block      = MakeBlock(uint64_t(-1), uint64_t(-2));
static const block                all_one_block      = MakeBlock(uint64_t(-1), uint64_t(-1));
static const std::array<block, 2> zero_and_all_one   = {zero_block, all_one_block};
static const block                all_bytes_one_mask = _mm_set1_epi8(0x01);

}    // namespace fsswm

#endif    // UTILS_BLOCK_H_
