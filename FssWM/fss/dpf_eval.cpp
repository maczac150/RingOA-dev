#include "dpf_eval.hpp"

#include "cryptoTools/Common/Defines.h"
#include <stack>

#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/utils.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

uint32_t DpfEvaluator::EvaluateAt(const DpfKey &key, uint32_t x) const {
    if (!ValidateInput(x)) {
        utils::Logger::FatalLog(LOC, "Invalid input value: x=" + std::to_string(x));
        exit(EXIT_FAILURE);
    }
    if (params_.GetEnableEarlyTermination()) {
        return EvaluateAtOptimized(key, x);
    } else {
        return EvaluateAtNaive(key, x);
    }
}

uint32_t DpfEvaluator::EvaluateAtNaive(const DpfKey &key, uint32_t x) const {
    uint32_t n = params_.GetInputBitsize();
    uint32_t e = params_.GetElementBitsize();

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate input with DPF key"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
    utils::Logger::DebugLog(LOC, "Input: " + std::to_string(x), debug_);
#endif

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Evaluate the DPF key
    std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
    std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

    for (uint32_t i = 0; i < n; i++) {
        EvaluateNextSeed(i, seed, control_bit, expanded_seeds, expanded_control_bits, key);

        // Update the seed and control bit based on the current bit
        bool current_bit = (x & (1U << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#ifdef LOG_LEVEL_DEBUG
        std::string level_str = "|Level=" + std::to_string(i) + "| ";
        utils::Logger::DebugLog(LOC, level_str + "Current bit: " + std::to_string(current_bit), debug_);
        utils::Logger::DebugLog(LOC, level_str + "Next seed: " + ToString(seed), debug_);
        utils::Logger::DebugLog(LOC, level_str + "Next control bit: " + std::to_string(control_bit), debug_);
#endif
    }
    // Compute the final output
    uint32_t output = utils::Pow(-1, key.party_id) * (Convert(seed, e) + (control_bit * Convert(key.output, e)));
    return utils::Mod(output, e);
}

uint32_t DpfEvaluator::EvaluateAtOptimized(const DpfKey &key, uint32_t x) const {
    uint32_t n  = params_.GetInputBitsize();
    uint32_t e  = params_.GetElementBitsize();
    uint32_t nu = params_.GetTerminateBitsize();

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate input with DPF key (optimized)"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
    utils::Logger::DebugLog(LOC, "Input: " + std::to_string(x), debug_);
#endif

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Evaluate the DPF key
    std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
    std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

    for (uint32_t i = 0; i < nu; i++) {
        EvaluateNextSeed(i, seed, control_bit, expanded_seeds, expanded_control_bits, key);

        // Update the seed and control bit based on the current bit
        bool current_bit = (x & (1U << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#ifdef LOG_LEVEL_DEBUG
        std::string level_str = "|Level=" + std::to_string(i) + "| ";
        utils::Logger::DebugLog(LOC, level_str + "Current bit: " + std::to_string(current_bit), debug_);
        utils::Logger::DebugLog(LOC, level_str + "Next seed: " + ToString(seed), debug_);
        utils::Logger::DebugLog(LOC, level_str + "Next control bit: " + std::to_string(control_bit), debug_);
#endif
    }

    // Compute the final output
    block    output_block = ComputeOutputBlock(seed, control_bit, key);
    uint32_t x_hat        = utils::GetLowerNBits(x, n - nu);
    uint32_t output       = GetValueFromSplitBlock(output_block, n - nu, x_hat);
    return utils::Mod(output, e);
}

bool DpfEvaluator::ValidateInput(const uint32_t x) const {
    bool valid = true;
    if (x >= (1UL << params_.GetInputBitsize())) {
        valid = false;
    }
    return valid;
}

void DpfEvaluator::EvaluateFullDomain(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t n         = params_.GetInputBitsize();
    uint32_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint32_t num_nodes = 1U << nu;

    // utils::TimerManager tm;
    // int32_t             tm_eval = tm.CreateNewTimer("Eval full domain");
    // int32_t             tm_out  = tm.CreateNewTimer("Compute output values");
    // tm.SelectTimer(tm_eval);
    // tm.Start();

    // Check if the domain size
    if (params_.GetElementBitsize() == 1) {
        utils::Logger::FatalLog(LOC, "You should use EvaluateFullDomainOneBit for the domain size of 1 bit");
        exit(EXIT_FAILURE);
    }

    // Check output vector size
    if (outputs.size() != 1U << params_.GetInputBitsize()) {
        outputs.resize(1U << params_.GetInputBitsize());
    }
    std::vector<block> outputs_block(num_nodes, zero_block);

    // Evaluate the DPF key for all possible x values
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
#endif
    switch (fde_type) {
        case EvalType::kNaive:
            FullDomainNaive(key, outputs);
            break;
        case EvalType::kRecursion:
            FullDomainRecursion(key, outputs_block);
            break;
        case EvalType::kIterSingle:
            FullDomainIterativeSingle(key, outputs_block);
            break;
        case EvalType::kIterDouble:
            FullDomainIterativeDouble(key, outputs_block);
            break;
        case EvalType::kIterSingleBatch:
            FullDomainIterativeSingleBatch(key, outputs_block);
            break;
        case EvalType::kIterDoubleBatch:
            FullDomainIterativeDoubleBatch(key, outputs_block);
            break;
        default:
            utils::Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(fde_type)));
            exit(EXIT_FAILURE);
    }
    // tm.Stop();
    // tm.SelectTimer(tm_out);
    // tm.Start();
    if (fde_type != EvalType::kNaive) {
        ConvertVector(outputs_block, n - nu, params_.GetElementBitsize(), outputs);
    }
    // tm.Stop("Compute output values");
    // tm.PrintAllResults(utils::TimeUnit::MICROSECONDS);
}

void DpfEvaluator::EvaluateFullDomainOneBit(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint32_t num_nodes = 1U << nu;

    // Check if the domain size
    if (params_.GetElementBitsize() != 1) {
        utils::Logger::FatalLog(LOC, "This function is only for the domain size of 1 bit");
        exit(EXIT_FAILURE);
    }

    // Check output vector size
    if (outputs.size() != num_nodes) {
        outputs.resize(num_nodes);
    }

    // Evaluate the DPF key for all possible x values
    switch (fde_type) {
        case EvalType::kNaive:
            utils::Logger::FatalLog(LOC, "Naive approach is not supported for the domain size of 1 bit");
            break;
        case EvalType::kRecursion:
            FullDomainRecursion(key, outputs);
            break;
        case EvalType::kIterSingle:
            FullDomainIterativeSingle(key, outputs);
            break;
        case EvalType::kIterDouble:
            FullDomainIterativeDouble(key, outputs);
            break;
        case EvalType::kIterSingleBatch:
            FullDomainIterativeSingleBatch(key, outputs);
            break;
        case EvalType::kIterDoubleBatch:
            FullDomainIterativeDoubleBatch(key, outputs);
            break;
        default:
            utils::Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(fde_type)));
            exit(EXIT_FAILURE);
    }
}

void DpfEvaluator::EvaluateNextSeed(
    const uint32_t current_level, const block &current_seed, const bool &current_control_bit,
    std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
    const DpfKey &key) const {
    // Expand the seed and control bits
    G_.DoubleExpand(current_seed, expanded_seeds);
    expanded_control_bits[kLeft]  = getLSB(expanded_seeds[kLeft]);
    expanded_control_bits[kRight] = getLSB(expanded_seeds[kRight]);

#ifdef LOG_LEVEL_DEBUG
    std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
    utils::Logger::DebugLog(LOC, level_str + "Current seed: " + ToString(current_seed), debug_);
    utils::Logger::DebugLog(LOC, level_str + "Current control bit: " + std::to_string(current_control_bit), debug_);
    utils::Logger::DebugLog(LOC, level_str + "Expanded seed (L): " + ToString(expanded_seeds[kLeft]), debug_);
    utils::Logger::DebugLog(LOC, level_str + "Expanded seed (R): " + ToString(expanded_seeds[kRight]), debug_);
    utils::Logger::DebugLog(LOC, level_str + "Expanded control bit (L, R): " + std::to_string(expanded_control_bits[kLeft]) + ", " + std::to_string(expanded_control_bits[kRight]), debug_);
#endif

    // Apply correction word if control bit is true
    const block mask = key.cw_seed[current_level] & zero_and_all_one[current_control_bit];
    expanded_seeds[kLeft] ^= mask;
    expanded_seeds[kRight] ^= mask;

    const bool control_mask_left  = key.cw_control_left[current_level] & current_control_bit;
    const bool control_mask_right = key.cw_control_right[current_level] & current_control_bit;
    expanded_control_bits[kLeft] ^= control_mask_left;
    expanded_control_bits[kRight] ^= control_mask_right;
}

void DpfEvaluator::FullDomainRecursion(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu = params_.GetTerminateBitsize();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    Traverse(seed, control_bit, key, nu, 0, outputs);
}

void DpfEvaluator::FullDomainIterativeSingle(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu = params_.GetTerminateBitsize();

    // Initialize the variables
    uint32_t           current_level = 0;
    uint32_t           current_idx   = 0;
    uint32_t           last_depth    = nu;
    uint32_t           last_idx      = 1U << last_depth;
    std::vector<block> output_seeds(last_idx, zero_block);
    std::vector<bool>  output_control_bits(last_idx, false);

    // Store the seeds and control bits
    block              expanded_seed;
    bool               expanded_control_bit;
    std::vector<block> prev_seeds(last_depth + 1);
    std::vector<bool>  prev_control_bits(last_depth + 1);

    // Evaluate the DPF key
    prev_seeds[0]        = key.init_seed;
    prev_control_bits[0] = key.party_id != 0;

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint32_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seed, current_bit);

            expanded_control_bit = getLSB(expanded_seed);

#ifdef LOG_LEVEL_DEBUG
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            utils::Logger::DebugLog(LOC, level_str + "Current bit: " + std::to_string(current_bit), debug_);
            utils::Logger::DebugLog(LOC, level_str + "Current seed: " + ToString(prev_seeds[current_level]), debug_);
            utils::Logger::DebugLog(LOC, level_str + "Current control bit: " + std::to_string(prev_control_bits[current_level]), debug_);
            utils::Logger::DebugLog(LOC, level_str + "Expanded seed: " + ToString(expanded_seed), debug_);
            utils::Logger::DebugLog(LOC, level_str + "Expanded control bit: " + std::to_string(expanded_control_bit), debug_);
#endif

            // Apply correction word if control bit is true
            bool cw_control_bit = current_bit ? key.cw_control_right[current_level] : key.cw_control_left[current_level];
            expanded_seed ^= (key.cw_seed[current_level] & zero_and_all_one[prev_control_bits[current_level]]);
            expanded_control_bit ^= (cw_control_bit & prev_control_bits[current_level]);

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            prev_seeds[current_level]        = expanded_seed;
            prev_control_bits[current_level] = expanded_control_bit;
        }

        // Get the seed and control bit from prev_seeds and prev_control_bits
        output_seeds[current_idx]        = prev_seeds[current_level];
        output_control_bits[current_idx] = prev_control_bits[current_level];

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= oc::log2floor(shift) + 1;
        current_idx++;
    }

#ifdef LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < last_idx; i++) {
        utils::Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(output_seeds[i]), debug_);
    }
#endif

    // Compute the output values
    outputs = ComputeOutputBlocks(output_seeds, output_control_bits, key);
}

void DpfEvaluator::FullDomainIterativeDouble(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu = params_.GetTerminateBitsize();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Initialize the variables
    uint32_t           current_level = 0;
    uint32_t           current_idx   = 0;
    uint32_t           last_depth    = nu;
    uint32_t           last_idx      = 1U << last_depth;
    std::vector<block> output_seeds(last_idx, zero_block);
    std::vector<bool>  output_control_bits(last_idx, false);

    // Store the expanded seeds and control bits
    std::array<block, 2> expanded_seeds;
    std::array<bool, 2>  expanded_control_bits;
    std::stack<block>    seed_stack;
    std::stack<bool>     control_bit_stack;

    // Evaluate the DPF key
    seed_stack.push(seed);
    control_bit_stack.push(control_bit);

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Get the seed and control bit from the queue
            EvaluateNextSeed(current_level, seed_stack.top(), control_bit_stack.top(), expanded_seeds, expanded_control_bits, key);
            seed_stack.pop();
            control_bit_stack.pop();

            // Push the expanded seeds and control bits to the queue
            seed_stack.push(expanded_seeds[kRight]);
            seed_stack.push(expanded_seeds[kLeft]);
            control_bit_stack.push(expanded_control_bits[kRight]);
            control_bit_stack.push(expanded_control_bits[kLeft]);

            // Update the current level
            current_level++;
        }

        // Get the seed and control bit from the queue
        for (int i = 0; i < 2; i++) {
            output_seeds[current_idx + i]        = seed_stack.top();
            output_control_bits[current_idx + i] = control_bit_stack.top();
            seed_stack.pop();
            control_bit_stack.pop();
        }

        // Update the current index
        current_idx += 2;
        int shift = (current_idx - 1U) ^ current_idx;
        current_level -= oc::log2floor(shift);
    }

#ifdef LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < last_idx; i++) {
        utils::Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(output_seeds[i]), debug_);
    }
#endif

    // Compute the output values
    outputs = ComputeOutputBlocks(output_seeds, output_control_bits, key);
}

void DpfEvaluator::FullDomainIterativeSingleBatch(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu            = params_.GetTerminateBitsize();
    uint32_t remaining_bit = params_.GetInputBitsize() - nu;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint32_t i = 0; i < 3; i++) {
        std::array<block, 2> expanded_seeds;
        std::array<bool, 2>  expanded_control_bits;
        next_seeds.resize(1U << (i + 1));
        next_control_bits.resize(1U << (i + 1));
        for (size_t j = 0; j < start_seeds.size(); j++) {
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[kRight];
            next_control_bits[j * 2]     = expanded_control_bits[kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[kRight];
        }
        start_seeds        = std::move(next_seeds);
        start_control_bits = std::move(next_control_bits);
    }

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint32_t last_idx      = 1U << last_depth;

    // Store the seeds and control bits
    std::array<block, 8>              expanded_seeds;
    std::array<bool, 8>               expanded_control_bits;
    std::vector<std::array<block, 8>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 8>>  prev_control_bits(last_depth + 1);

    // Evaluate the DPF key
    for (uint32_t i = 0; i < 8; i++) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint32_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint32_t i = 0; i < 8; i++) {
                expanded_control_bits[i] = getLSB(expanded_seeds[i]);
            }

#ifdef LOG_LEVEL_DEBUG
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 8; i++) {
                utils::Logger::DebugLog(LOC, level_str + "Current bit: " + std::to_string(current_bit), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(prev_seeds[current_level][i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(prev_control_bits[current_level][i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded seed (" + std::to_string(i) + "): " + ToString(expanded_seeds[i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded control bit (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i]), debug_);
            }
#endif

            // Apply correction word if control bit is true
            bool cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            for (uint32_t i = 0; i < 8; i++) {
                expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint32_t i = 0; i < 8; i++) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        if (remaining_bit == 2) {
            if (key.party_id) {
                for (uint32_t i = 0; i < 8; i++) {
                    outputs[i * last_idx + current_idx] = _mm_sub_epi32(zero_block, _mm_add_epi32(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output));
                }
            } else {
                for (uint32_t i = 0; i < 8; i++) {
                    outputs[i * last_idx + current_idx] = _mm_add_epi32(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output);
                }
            }
        } else if (remaining_bit == 3) {
            if (key.party_id) {
                for (uint32_t i = 0; i < 8; i++) {
                    outputs[i * last_idx + current_idx] = _mm_sub_epi16(zero_block, _mm_add_epi16(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output));
                }
            } else {
                for (uint32_t i = 0; i < 8; i++) {
                    outputs[i * last_idx + current_idx] = _mm_add_epi16(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output);
                }
            }
        } else if (remaining_bit == 7) {
            for (uint32_t i = 0; i < 8; i++) {
                outputs[i * last_idx + current_idx] = prev_seeds[current_level][i] ^ (zero_and_all_one[prev_control_bits[current_level][i]] & key.output);
            }
        } else {
            utils::Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            exit(EXIT_FAILURE);
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= oc::log2floor(shift) + 1;
        current_idx++;
    }

#ifdef LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < outputs.size(); i++) {
        utils::Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(outputs[i]), debug_);
    }
#endif
}

void DpfEvaluator::FullDomainIterativeDoubleBatch(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu            = params_.GetTerminateBitsize();
    uint32_t num_nodes     = 1U << nu;
    uint32_t remaining_bit = params_.GetInputBitsize() - nu;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint32_t i = 0; i < 3; i++) {
        std::array<block, 2> expanded_seeds;
        std::array<bool, 2>  expanded_control_bits;
        next_seeds.resize(1U << (i + 1));
        next_control_bits.resize(1U << (i + 1));
        for (size_t j = 0; j < start_seeds.size(); j++) {
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[kRight];
            next_control_bits[j * 2]     = expanded_control_bits[kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[kRight];
        }
        start_seeds        = std::move(next_seeds);
        start_control_bits = std::move(next_control_bits);
    }

    // Push the initial seeds and control bits to the queue
    std::array<block, 8> current_seeds;
    std::array<bool, 8>  current_control_bits;
    std::copy(start_seeds.begin(), start_seeds.end(), current_seeds.begin());
    std::copy(start_control_bits.begin(), start_control_bits.end(), current_control_bits.begin());

    // Initialize the variables
    uint32_t           current_level = 0;
    uint32_t           current_idx   = 0;
    uint32_t           last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint32_t           last_idx      = 1U << last_depth;
    std::vector<block> output_seeds(num_nodes, zero_block);
    std::vector<bool>  output_control_bits(num_nodes, false);

    // Store the expanded seeds and control bits
    std::array<std::array<block, 8>, 2> expanded_seeds;
    std::array<std::array<bool, 8>, 2>  expanded_control_bits;
    std::stack<std::array<block, 8>>    seed_stacks;
    std::stack<std::array<bool, 8>>     control_bit_stacks;

    // Evaluate the DPF key
    seed_stacks.push(current_seeds);
    control_bit_stacks.push(current_control_bits);

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Get the seed and control bit from the queue
            current_seeds        = seed_stacks.top();
            current_control_bits = control_bit_stacks.top();
            seed_stacks.pop();
            control_bit_stacks.pop();

            // Expand the seed and control bits
            G_.DoubleExpand(current_seeds, expanded_seeds);
            for (uint32_t i = 0; i < 8; i++) {
                expanded_control_bits[kLeft][i]  = getLSB(expanded_seeds[kLeft][i]);
                expanded_control_bits[kRight][i] = getLSB(expanded_seeds[kRight][i]);
            }

#ifdef LOG_LEVEL_DEBUG
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 8; i++) {
                utils::Logger::DebugLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(current_seeds[i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(current_control_bits[i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded seed (L) (" + std::to_string(i) + "): " + ToString(expanded_seeds[kLeft][i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded seed (R) (" + std::to_string(i) + "): " + ToString(expanded_seeds[kRight][i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded control bit (L, R) (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i][kLeft]) + ", " + std::to_string(expanded_control_bits[i][kRight]), debug_);
            }
#endif

            // Apply correction word if control bit is true
            for (uint32_t i = 0; i < 8; i++) {
                expanded_seeds[kLeft][i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[current_control_bits[i]]);
                expanded_seeds[kRight][i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[current_control_bits[i]]);
                expanded_control_bits[kLeft][i] ^= (key.cw_control_left[current_level + 3] & current_control_bits[i]);
                expanded_control_bits[kRight][i] ^= (key.cw_control_right[current_level + 3] & current_control_bits[i]);
            }

            // Push the expanded seeds and control bits to the queue
            seed_stacks.push(expanded_seeds[kRight]);
            seed_stacks.push(expanded_seeds[kLeft]);
            control_bit_stacks.push(expanded_control_bits[kRight]);
            control_bit_stacks.push(expanded_control_bits[kLeft]);

            // Update the current level
            current_level++;
        }

        if (remaining_bit == 2) {
            for (uint32_t i = 0; i < 2; i++) {
                auto &tmp_seeds        = seed_stacks.top();
                auto &tmp_control_bits = control_bit_stacks.top();
                if (key.party_id) {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_sub_epi32(zero_block, _mm_add_epi32(tmp_seeds[j], zero_and_all_one[tmp_control_bits[j]] & key.output));
                    }
                } else {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_add_epi32(tmp_seeds[j], zero_and_all_one[tmp_control_bits[j]] & key.output);
                    }
                }
                seed_stacks.pop();
                control_bit_stacks.pop();
            }
        } else if (remaining_bit == 3) {
            for (uint32_t i = 0; i < 2; i++) {
                auto &tmp_seeds        = seed_stacks.top();
                auto &tmp_control_bits = control_bit_stacks.top();
                if (key.party_id) {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_sub_epi16(zero_block, _mm_add_epi16(tmp_seeds[j], zero_and_all_one[tmp_control_bits[j]] & key.output));
                    }
                } else {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_add_epi16(tmp_seeds[j], zero_and_all_one[tmp_control_bits[j]] & key.output);
                    }
                }
                seed_stacks.pop();
                control_bit_stacks.pop();
            }
        } else if (remaining_bit == 7) {
            for (uint32_t i = 0; i < 2; i++) {
                auto &tmp_seeds        = seed_stacks.top();
                auto &tmp_control_bits = control_bit_stacks.top();
                for (uint32_t j = 0; j < 8; j++) {
                    outputs[j * last_idx + current_idx + i] = tmp_seeds[j] ^ (zero_and_all_one[tmp_control_bits[j]] & key.output);
                }
                seed_stacks.pop();
                control_bit_stacks.pop();
            }
        } else {
            utils::Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            exit(EXIT_FAILURE);
        }

        // Update the current index
        current_idx += 2;
        int shift = (current_idx - 1U) ^ current_idx;
        current_level -= oc::log2floor(shift);
    }

#ifdef LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < num_nodes; i++) {
        utils::Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(outputs[i]), debug_);
    }
#endif
}

void DpfEvaluator::FullDomainNaive(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    // Evaluate the DPF key for all possible x values
    for (uint32_t x = 0; x < (1U << params_.GetInputBitsize()); x++) {
        outputs[x] = EvaluateAtNaive(key, x);
    }
}

void DpfEvaluator::Traverse(const block &current_seed, const bool current_control_bit, const DpfKey &key, uint32_t i, uint32_t j, std::vector<block> &outputs) const {
    uint32_t nu = params_.GetTerminateBitsize();

    if (i > 0) {
        // Evaluate the DPF key
        alignas(16) std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
        alignas(16) std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

        EvaluateNextSeed(nu - i, current_seed, current_control_bit, expanded_seeds, expanded_control_bits, key);

        // Traverse the left and right subtrees
        Traverse(expanded_seeds[kLeft], expanded_control_bits[kLeft], key, i - 1, j, outputs);
        Traverse(expanded_seeds[kRight], expanded_control_bits[kRight], key, i - 1, j + (1U << (i - 1)), outputs);
    } else {
        // Compute the output block
        outputs[j] = ComputeOutputBlock(current_seed, current_control_bit, key);
    }
}

block DpfEvaluator::ComputeOutputBlock(const block &final_seed, bool final_control_bit, const DpfKey &key) const {
    // Compute the mask block and remaining bits
    block    mask          = zero_and_all_one[final_control_bit];
    uint32_t remaining_bit = params_.GetInputBitsize() - params_.GetTerminateBitsize();

    // Set the output block
    block output = zero_block;
    if (remaining_bit == 2) {
        // Reduce 2 levels (2^2=4 nodes) of the tree (Additive share)
        if (key.party_id) {
            output = _mm_sub_epi32(zero_block, _mm_add_epi32(final_seed, mask & key.output));
        } else {
            output = _mm_add_epi32(final_seed, mask & key.output);
        }
    } else if (remaining_bit == 3) {
        // Reduce 3 levels (2^3=8 nodes) of the tree (Additive share)
        if (key.party_id) {
            output = _mm_sub_epi16(zero_block, _mm_add_epi16(final_seed, mask & key.output));
        } else {
            output = _mm_add_epi16(final_seed, mask & key.output);
        }
    } else if (remaining_bit == 7) {
        // Reduce 7 levels (2^7=128 nodes) of the tree (Additive share)
        output = final_seed ^ (mask & key.output);
    } else {
        utils::Logger::FatalLog(LOC, "Unsupported termination bitsize: " + std::to_string(remaining_bit));
        exit(EXIT_FAILURE);
    }
    return output;
}

std::vector<block> DpfEvaluator::ComputeOutputBlocks(const std::vector<block> &final_seeds, const std::vector<bool> &final_control_bits, const DpfKey &key) const {
    // Compute the mask block and remaining bits
    uint32_t remaining_bit = params_.GetInputBitsize() - params_.GetTerminateBitsize();

    // Set the output block
    std::vector<block> outputs(final_seeds.size(), zero_block);
    if (remaining_bit == 2) {
        if (key.party_id) {
            // Reduce 2 levels (2^2=4 nodes) of the tree (Additive share)
            for (uint32_t i = 0; i < final_seeds.size(); i++) {
                outputs[i] = _mm_sub_epi32(zero_block, _mm_add_epi32(final_seeds[i], zero_and_all_one[final_control_bits[i]] & key.output));
            }
        } else {
            for (uint32_t i = 0; i < final_seeds.size(); i++) {
                outputs[i] = _mm_add_epi32(final_seeds[i], zero_and_all_one[final_control_bits[i]] & key.output);
            }
        }
    } else if (remaining_bit == 3) {
        // Reduce 3 levels (2^3=8 nodes) of the tree (Additive share)
        if (key.party_id) {
            for (uint32_t i = 0; i < final_seeds.size(); i++) {
                outputs[i] = _mm_sub_epi16(zero_block, _mm_add_epi16(final_seeds[i], zero_and_all_one[final_control_bits[i]] & key.output));
            }
        } else {
            for (uint32_t i = 0; i < final_seeds.size(); i++) {
                outputs[i] = _mm_add_epi16(final_seeds[i], zero_and_all_one[final_control_bits[i]] & key.output);
            }
        }
    } else if (remaining_bit == 7) {
        // Reduce 7 levels (2^7=128 nodes) of the tree (Additive share)
        for (uint32_t i = 0; i < final_seeds.size(); i++) {
            outputs[i] = final_seeds[i] ^ (zero_and_all_one[final_control_bits[i]] & key.output);
        }
    } else {
        utils::Logger::FatalLog(LOC, "Unsupported termination bitsize: " + std::to_string(remaining_bit));
        exit(EXIT_FAILURE);
    }
    return outputs;
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
