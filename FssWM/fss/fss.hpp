#ifndef FSS_FSS_H_
#define FSS_FSS_H_

#include <emp-tool/emp-tool.h>

#include "utils/logger.hpp"

using namespace emp;

namespace fsswm {

namespace fss {

const block                zero_block       = makeBlock(0, 0);
const block                one_block        = makeBlock(0, 1);
const block                all_one_block    = makeBlock(uint64_t(-1), uint64_t(-1));
const std::array<block, 2> zero_and_all_one = {zero_block, all_one_block};

constexpr uint32_t kLeft  = 0;
constexpr uint32_t kRight = 1;

// Define format types
enum class FormatType
{
    kBin,    // Binary
    kHex,    // Hexadecimal
    kDec     // Decimal
};

/**
 * @brief Converts a block to a std::string representation.
 *
 * This function takes a block and converts it into a human-readable std::string format.
 *
 * @param b The block to be converted.
 * @param format The format type of the output string.
 * @return A std::string representation of the block.
 */
std::string ToString(const block &blk, FormatType format = FormatType::kHex);

/**
 * @brief Compares two blocks for equality.
 *
 * This function compares two blocks for equality.
 *
 * @param lhs The first block to be compared.
 * @param rhs The second block to be compared.
 * @return A boolean value indicating whether the two blocks are equal.
 */
bool Equal(const block &lhs, const block &rhs);

/**
 * @brief Sets a random block.
 *
 * This function generates a random block.
 *
 * @return A random block.
 */
block SetRandomBlock();

/**
 * @brief Converts a block to a uint32_t value.
 *
 * This function converts a block to a uint32_t value with the specified bitsize.
 *
 * @param b The block to be converted.
 * @param bitsize The bitsize of the output value.
 * @return A uint32_t value converted from the block.
 */
uint32_t Convert(const block &b, const uint32_t bitsize);

}    // namespace fss
}    // namespace fsswm

#endif    // FSS_FSS_H_
