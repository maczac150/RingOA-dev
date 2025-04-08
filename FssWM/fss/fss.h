#ifndef FSS_FSS_H_
#define FSS_FSS_H_

#include <cstdint>
#include <vector>

#include "FssWM/utils/block.h"

namespace fsswm {

enum class ShareType
{
    kAdditive,
    kBinary,
};

namespace fss {

constexpr uint32_t kSecurityParameter = 128;
constexpr uint32_t kLeft              = 0;
constexpr uint32_t kRight             = 1;
constexpr uint32_t kSmallDomainSize   = 8;

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
