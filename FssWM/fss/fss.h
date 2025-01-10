#ifndef FSS_FSS_H_
#define FSS_FSS_H_

#include <array>
#include <cstdint>
#include <immintrin.h>
#include <string>
#include <vector>

namespace fsswm {

namespace fss {

using block = __m128i;

inline block makeBlock(uint64_t high, uint64_t low) {
    return _mm_set_epi64x(high, low);
}

inline bool getLSB(const block &x) {
    return (x[0] & 1) == 1;
}

inline int log2floor(uint32_t x) {
    return x == 0 ? -1 : 31 - __builtin_clz(x);
}

const block                zero_block       = makeBlock(0, 0);
const block                one_block        = makeBlock(0, 1);
const block                all_one_block    = makeBlock(uint64_t(-1), uint64_t(-1));
const std::array<block, 2> zero_and_all_one = {zero_block, all_one_block};

constexpr uint32_t kSecurityParameter = 128;
constexpr uint32_t kLeft              = 0;
constexpr uint32_t kRight             = 1;
constexpr uint32_t kSmallDomainSize   = 8;

// Define format types
enum class FormatType
{
    kBin,    // Binary
    kHex,    // Hexadecimal
    kDec     // Decimal
};

/**
 * @brief Get the least significant bit of a block.
 * @param x The block to be checked.
 * @return The least significant bit of the block.
 */
bool getLSB(const block &x);

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

/**
 * @brief Converts a block to a uint32_t value.
 * @param b The block to be converted.
 * @param bitsize The bitsize of the output value.
 * @return A uint32_t value converted from the block.
 */
uint32_t Convert(const block &b, const uint32_t bitsize);

/**
 * @brief Splits a block into some blocks and covert them into a vector.
 * @param b The block to be converted.
 * @param split_bit The number of bits to split the block into.
 * @param bitsize The bitsize of the output value.
 * @param output The vector to store the output values.
 */
void ConvertVector(const block &b, const uint32_t split_bit, const uint32_t bitsize, std::vector<uint32_t> &output);

/**
 * @brief Splits a vector of blocks into some blocks and covert them into a vector.
 * @param b The vector of blocks to be converted.
 * @param split_bit The number of bits to split the block into.
 * @param bitsize The bitsize of the output value.
 * @param output The vector to store the output values.
 */
void ConvertVector(const std::vector<block> &b, const uint32_t split_bit, const uint32_t bitsize, std::vector<uint32_t> &output);

void ConvertVector(const std::vector<block> &b1,
                   const std::vector<block> &b2,
                   const uint32_t            split_bit,
                   const uint32_t            bitsize,
                   std::vector<uint32_t>    &out1,
                   std::vector<uint32_t>    &out2);

/**
 * @brief Split a block into some blocks and get the `idx`-th value.
 * @param b The block to be split.
 * @param split_bit The number of bits to split the block into.
 * @param idx The index of the block to retrieve.
 * @return The `idx`-th block after splitting `b` into `num` blocks.
 */
uint32_t GetValueFromSplitBlock(block &b, const uint32_t split_bit, const uint32_t idx);

}    // namespace fss
}    // namespace fsswm

#endif    // FSS_FSS_H_
