#ifndef UTILS_BLOCK_H_
#define UTILS_BLOCK_H_

#include <array>
#include <immintrin.h>

#include "utils.h"

namespace fsswm {

using block = __m128i;

/**
 * @brief Create a block from two 64-bit integers.
 * @param high The higher 64-bit integer.
 * @param low The lower 64-bit integer.
 * @return The block created from the two integers.
 */
inline block makeBlock(uint64_t high, uint64_t low) {
    return _mm_set_epi64x(high, low);
}

/**
 * @brief Get the least significant bit of a block.
 * @param x The block to be checked.
 * @return The least significant bit of the block.
 */
inline bool getLSB(const block &x) {
    return (x[0] & 1) == 1;
}

/**
 * @brief Get the most significant bit of a block.
 * @param x The block to be checked.
 * @return The most significant bit of the block.
 */
inline int log2floor(uint32_t x) {
    return x == 0 ? -1 : 31 - __builtin_clz(x);
}

const block                zero_block       = makeBlock(0, 0);
const block                one_block        = makeBlock(0, 1);
const block                all_one_block    = makeBlock(uint64_t(-1), uint64_t(-1));
const std::array<block, 2> zero_and_all_one = {zero_block, all_one_block};

/**
 * @brief Converts a block to a std::string representation.
 * @param b The block to be converted.
 * @param format The format type of the output string.
 * @return A std::string representation of the block.
 */
std::string ToString(const block &blk, FormatType format = FormatType::kHex);

/**
 * @brief Compares two blocks for equality.
 * @param lhs The first block to be compared.
 * @param rhs The second block to be compared.
 * @return A boolean value indicating whether the two blocks are equal.
 */
bool Equal(const block &lhs, const block &rhs);

/**
 * @brief Sets a random block.
 * @return A random block.
 */
block SetRandomBlock();

}    // namespace fsswm

#endif    // UTILS_BLOCK_H_
