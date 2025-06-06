#ifndef FSS_FSS_H_
#define FSS_FSS_H_

#include <cstdint>
#include <vector>

#include "FssWM/utils/block.h"

namespace fsswm {
namespace fss {

constexpr uint64_t kSecurityParameter = 128;
constexpr uint64_t kLeft              = 0;
constexpr uint64_t kRight             = 1;

/**
 * @brief Enumeration for the evaluation types for DPF.
 */
enum class EvalType
{
    kNaive,
    kRecursion,
    kIterSingleBatch,
};

/**
 * @brief Output modes for the Distributed Point Function (DPF).
 */
enum class OutputType
{
    kShiftedAdditive,
    kSingleBitMask,
};

const EvalType kOptimizedEvalType = EvalType::kIterSingleBatch;

/**
 * @brief Get the string representation of the evaluation type.
 * @param eval_type The evaluation type for the DPF key.
 * @return std::string The string representation of the evaluation type.
 */
std::string GetEvalTypeString(const EvalType eval_type);
std::string GetOutputTypeString(const OutputType mode);

/**
 * @brief Converts a block to a uint64_t value.
 * @param b The block to be converted.
 * @param bitsize The bitsize of the output value.
 * @return A uint64_t value converted from the block.
 */
uint64_t Convert(const block &b, const uint64_t bitsize);

/**
 * @brief Splits a vector of 128-bit blocks into 2^chunk_exp elements per block, masks each element with field_bits width, and stores them in a uint64_t vector.
 * @param blks        Input vector of blocks.
 * @param chunk_exp   Exponent for the number of elements (number of elements = 1 << chunk_exp). Only 2, 3, and 7 are supported (4, 8, 128 elements).
 * @param field_bits  Bit width to mask each element (1–64). If 64, all bits are valid.
 * @param out         Output vector. The size is automatically adjusted.
 */
void SplitBlockToFieldVector(const std::vector<block> &blks,
                             uint64_t                  chunk_exp,
                             uint64_t                  field_bits,
                             std::vector<uint64_t>    &out);

/**
 * @brief Splits a 128-bit block into 2^chunk_exp elements and returns the specified element as a uint64_t.
 * @param blk          Input 128-bit block
 * @param chunk_exp    Split exponent (2 → 4 elements, 3 → 8 elements, 7 → 128 elements)
 * @param element_idx  Index of the element to retrieve [0, 2^chunk_exp)
 * @return             The value of the element as uint64_t
 */
uint64_t GetSplitBlockValue(const block &blk,
                            uint64_t     chunk_exp,
                            uint64_t     element_idx,
                            OutputType   mode);

}    // namespace fss
}    // namespace fsswm

#endif    // FSS_FSS_H_
