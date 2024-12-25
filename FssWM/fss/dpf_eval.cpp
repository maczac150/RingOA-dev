#include "dpf_eval.hpp"

#include "cryptoTools/Common/Defines.h"
#include <stack>

#include "utils/logger.hpp"
#include "utils/utils.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

uint32_t DpfEvaluator::EvaluateAt(const DpfKey &key, uint32_t x) const {
    if (!ValidateInput(x)) {
        utils::Logger::FatalLog(LOC, "Invalid input value: x=" + std::to_string(x));
        exit(EXIT_FAILURE);
    }
    if (params_.enable_early_termination) {
        return EvaluateAtOptimized(key, x);
    } else {
        return EvaluateAtNaive(key, x);
    }
}

uint32_t DpfEvaluator::EvaluateAtNaive(const DpfKey &key, uint32_t x) const {
    uint32_t n = params_.input_bitsize;
    uint32_t e = params_.element_bitsize;

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
    uint32_t n  = params_.input_bitsize;
    uint32_t e  = params_.element_bitsize;
    uint32_t nu = params_.terminate_bitsize;

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
    if (x >= (1UL << params_.input_bitsize)) {
        valid = false;
    }
    return valid;
}

std::vector<uint32_t> DpfEvaluator::EvaluateFullDomain(const DpfKey &key, const EvalType eval_type) const {
    // Initialize the output vector
    std::vector<uint32_t> outputs(1U << params_.input_bitsize);

    // Evaluate the DPF key for all possible x values
    if (params_.input_bitsize <= kSmallDomainSize) {
        FullDomainNaive(key, outputs);
    } else {
        if (params_.enable_early_termination) {
            EvaluateFullDomainOptimized(key, outputs, eval_type);
        } else {
            utils::Logger::FatalLog(LOC, "Set the early termination flag to true except for small domain size");
            exit(EXIT_FAILURE);
        }
    }
    return outputs;
}

void DpfEvaluator::EvaluateFullDomainOptimized(const DpfKey &key, std::vector<uint32_t> &outputs, const EvalType eval_type) const {
    uint32_t remaining_bit = params_.input_bitsize - params_.terminate_bitsize;

    switch (eval_type) {
        case EvalType::kRecursive:
            FullDomainRecursive(key, outputs);
            break;
        case EvalType::kNonRecursive:
            FullDomainNonRecursive(key, outputs);
            break;
        case EvalType::kNonRecursive_Opt:
            if (remaining_bit == 2) {
                FullDomainNonRecursive_Opt4(key, outputs);
            } else if (remaining_bit == 3) {
                FullDomainNonRecursive_Opt8(key, outputs);
            } else {
                utils::Logger::FatalLog(LOC, "Invalid remaining bitsize: " + std::to_string(remaining_bit));
                exit(EXIT_FAILURE);
            }
            break;
        case EvalType::kBfs:
            FullDomainBfs(key, outputs);
            break;
        default:
            utils::Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(eval_type)));
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
    expanded_seeds[kLeft] ^= (key.cw_seed[current_level] & zero_and_all_one[current_control_bit]);
    expanded_seeds[kRight] ^= (key.cw_seed[current_level] & zero_and_all_one[current_control_bit]);
    expanded_control_bits[kLeft] ^= (key.cw_control_left[current_level] & current_control_bit);
    expanded_control_bits[kRight] ^= (key.cw_control_right[current_level] & current_control_bit);
}

void DpfEvaluator::FullDomainRecursive(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t nu = params_.terminate_bitsize;

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate full domain with DPF key (recursive)"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
#endif

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    Traverse(seed, control_bit, key, nu, 0, outputs);
}

void DpfEvaluator::FullDomainNonRecursive(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t n               = params_.input_bitsize;
    uint32_t nu              = params_.terminate_bitsize;
    uint32_t remaining_nodes = 1U << (n - nu);

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate full domain with DPF key (non-recursive)"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
#endif

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = nu;
    uint32_t last_idx      = 1U << last_depth;
    if (outputs.size() != (1U << n)) {
        utils::Logger::FatalLog(LOC, "Output vector size does not match the expected size: " + std::to_string(outputs.size()) + " != " + std::to_string(last_idx));
        exit(EXIT_FAILURE);
    }

    // Evaluate the DPF key
    std::array<block, 2> expanded_seeds;
    std::array<bool, 2>  expanded_control_bits;
    std::stack<block>    seed_stack;
    std::stack<bool>     control_bit_stack;
    std::vector<block>   output_seeds(last_idx, zero_block);
    std::vector<bool>    output_control_bits(last_idx, false);

    seed_stack.push(seed);
    control_bit_stack.push(control_bit);

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            utils::Logger::DebugLog(LOC, "Idx: " + std::to_string(current_idx) + " | Depth: " + std::to_string(current_level), debug_);

            // Get the seed and control bit from the queue
            seed        = seed_stack.top();
            control_bit = control_bit_stack.top();
            seed_stack.pop();
            control_bit_stack.pop();

            EvaluateNextSeed(current_level, seed, control_bit, expanded_seeds, expanded_control_bits, key);

            // Push the expanded seeds and control bits to the queue
            seed_stack.push(expanded_seeds[kRight]);
            seed_stack.push(expanded_seeds[kLeft]);
            control_bit_stack.push(expanded_control_bits[kRight]);
            control_bit_stack.push(expanded_control_bits[kLeft]);

            current_level++;
        }

        for (int i = 0; i < 2; i++) {
            seed        = seed_stack.top();
            control_bit = control_bit_stack.top();
            seed_stack.pop();
            control_bit_stack.pop();

            output_seeds[current_idx + i]        = seed;
            output_control_bits[current_idx + i] = control_bit;
        }

        current_idx += 2;

        int shift = (current_idx - 1U) ^ current_idx;
        current_level -= oc::log2floor(shift);
    }

    // Print output seeds
    for (uint32_t i = 0; i < last_idx; i++) {
        utils::Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(output_seeds[i]), debug_);
    }

    // Compute the output values
    std::vector<block> outputs_block = ComputeOutputBlocks(output_seeds, output_control_bits, key);
    for (uint32_t i = 0; i < last_idx; i++) {
        std::vector<uint32_t> output_values = CovertVector(outputs_block[i], n - nu, params_.element_bitsize);
        for (uint32_t j = 0; j < remaining_nodes; j++) {
            outputs[i * remaining_nodes + j] = output_values[j];
        }
    }
}

void DpfEvaluator::FullDomainNonRecursive_Opt4(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t n               = params_.input_bitsize;
    uint32_t nu              = params_.terminate_bitsize;
    uint32_t remaining_nodes = 1U << (n - nu);

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate full domain with DPF key (non-recursive: 4)"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
#endif

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;
    for (uint32_t i = 0; i < 3; i++) {
        next_seeds.resize(1U << i + 1);
        next_control_bits.resize(1U << i + 1);
        for (size_t j = 0; j < start_seeds.size(); j++) {
            std::array<block, 2> expanded_seeds;
            std::array<bool, 2>  expanded_control_bits;
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[kRight];
            next_control_bits[j * 2]     = expanded_control_bits[kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[kRight];
        }
        std::swap(start_seeds, next_seeds);
        std::swap(start_control_bits, next_control_bits);
    }

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = nu - 3;
    uint32_t last_idx      = 1U << last_depth;
    if (outputs.size() != (1U << n)) {
        utils::Logger::FatalLog(LOC, "Output vector size does not match the expected size: " + std::to_string(outputs.size()) + " != " + std::to_string(last_idx));
        exit(EXIT_FAILURE);
    }

    // Evaluate the DPF key
    std::array<std::array<block, 2>, 8> expanded_seeds;
    std::array<std::array<bool, 2>, 8>  expanded_control_bits;
    std::array<std::stack<block>, 8>    seed_stacks;
    std::array<std::stack<bool>, 8>     control_bit_stacks;
    std::vector<block>                  output_seeds(last_idx, zero_block);
    std::vector<bool>                   output_control_bits(last_idx, false);
    std::array<block, 8>                current_seeds;
    std::array<bool, 8>                 current_control_bits;

    for (uint32_t i = 0; i < 8; i++) {
        seed_stacks[i].push(start_seeds[i]);
        control_bit_stacks[i].push(start_control_bits[i]);
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            utils::Logger::DebugLog(LOC, "Idx: " + std::to_string(current_idx) + " | Depth: " + std::to_string(current_level), debug_);

            // Get the seed and control bit from the queue
            for (uint32_t i = 0; i < 8; i++) {
                current_seeds[i]        = seed_stacks[i].top();
                current_control_bits[i] = control_bit_stacks[i].top();
                seed_stacks[i].pop();
                control_bit_stacks[i].pop();
            }

            // Expand the seed and control bits
            G_.DoubleExpand(current_seeds, expanded_seeds);
            for (uint32_t i = 0; i < 8; i++) {
                expanded_control_bits[i][kLeft]  = getLSB(expanded_seeds[i][kLeft]);
                expanded_control_bits[i][kRight] = getLSB(expanded_seeds[i][kRight]);
            }

#ifdef LOG_LEVEL_DEBUG
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 8; i++) {
                utils::Logger::DebugLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(current_seeds[i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(current_control_bits[i]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded seed (L, R) (" + std::to_string(i) + "): " + ToString(expanded_seeds[i][kLeft]) + ", " + ToString(expanded_seeds[i][kRight]), debug_);
                utils::Logger::DebugLog(LOC, level_str + "Expanded control bit (L, R) (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i][kLeft]) + ", " + std::to_string(expanded_control_bits[i][kRight]), debug_);
            }
#endif

            // Apply correction word if control bit is true
            for (uint32_t i = 0; i < 8; i++) {
                expanded_seeds[i][kLeft] ^= (key.cw_seed[current_level] & zero_and_all_one[current_control_bits[i]]);
                expanded_seeds[i][kRight] ^= (key.cw_seed[current_level] & zero_and_all_one[current_control_bits[i]]);
                expanded_control_bits[i][kLeft] ^= (key.cw_control_left[current_level] & current_control_bits[i]);
                expanded_control_bits[i][kRight] ^= (key.cw_control_right[current_level] & current_control_bits[i]);
            }

            // Push the expanded seeds and control bits to the queue
            for (uint32_t i = 0; i < 8; i++) {
                seed_stacks[i].push(expanded_seeds[i][kRight]);
                seed_stacks[i].push(expanded_seeds[i][kLeft]);
                control_bit_stacks[i].push(expanded_control_bits[i][kRight]);
                control_bit_stacks[i].push(expanded_control_bits[i][kLeft]);
            }

            current_level++;
        }

        for (uint32_t i = 0; i < 2; i++) {
            for (uint32_t j = 0; j < 8; j++) {
                current_seeds[j]        = seed_stacks[j].top();
                current_control_bits[j] = control_bit_stacks[j].top();
                seed_stacks[j].pop();
                control_bit_stacks[j].pop();

                output_seeds[current_idx + i]        = current_seeds[j];
                output_control_bits[current_idx + i] = current_control_bits[j];
            }
        }

        current_idx += 2;

        int shift = (current_idx - 1U) ^ current_idx;
        current_level -= oc::log2floor(shift);
    }
}

void DpfEvaluator::FullDomainNonRecursive_Opt8(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t n               = params_.input_bitsize;
    uint32_t nu              = params_.terminate_bitsize;
    uint32_t remaining_nodes = 1U << (n - nu);
}

void DpfEvaluator::FullDomainBfs(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t n               = params_.input_bitsize;
    uint32_t nu              = params_.terminate_bitsize;
    uint32_t remaining_nodes = 1U << (n - nu);

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate full domain with DPF key (non-recursive: 4)"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
#endif

    // Evaluate the DPF key
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;
    for (uint32_t i = 0; i < nu; i++) {
        next_seeds.resize(1U << i + 1);
        next_control_bits.resize(1U << i + 1);
        for (size_t j = 0; j < start_seeds.size(); j++) {
            std::array<block, 2> expanded_seeds;
            std::array<bool, 2>  expanded_control_bits;
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[kRight];
            next_control_bits[j * 2]     = expanded_control_bits[kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[kRight];
        }
        std::swap(start_seeds, next_seeds);
        std::swap(start_control_bits, next_control_bits);
    }

    // Compute the output values
    std::vector<block> outputs_block = ComputeOutputBlocks(next_seeds, next_control_bits, key);
    for (uint32_t i = 0; i < 1U << nu; i++) {
        std::vector<uint32_t> output_values = CovertVector(outputs_block[i], n - nu, params_.element_bitsize);
        for (uint32_t j = 0; j < remaining_nodes; j++) {
            outputs[i * remaining_nodes + j] = output_values[j];
        }
    }
}

void DpfEvaluator::FullDomainNaive(const DpfKey &key, std::vector<uint32_t> &outputs) const {
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Evaluate full domain with DPF key (naive)"), debug_);
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id), debug_);
#endif

    // Evaluate the DPF key for all possible x values
    for (uint32_t x = 0; x < (1U << params_.input_bitsize); x++) {
        outputs[x] = EvaluateAtNaive(key, x);
    }
}

void DpfEvaluator::Traverse(const block &current_seed, const bool current_control_bit, const DpfKey &key, uint32_t i, uint32_t j, std::vector<uint32_t> &outputs) const {
    uint32_t nu            = params_.terminate_bitsize;
    uint32_t remaining_bit = params_.input_bitsize - params_.terminate_bitsize;

    if (i > 0) {
        // Evaluate the DPF key
        std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
        std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

        EvaluateNextSeed(nu - i, current_seed, current_control_bit, expanded_seeds, expanded_control_bits, key);

        // Traverse the left and right subtrees
        Traverse(expanded_seeds[kLeft], expanded_control_bits[kLeft], key, i - 1, j, outputs);
        Traverse(expanded_seeds[kRight], expanded_control_bits[kRight], key, i - 1, j + utils::Pow(2, remaining_bit + i - 1), outputs);
    } else {
        utils::Logger::DebugLog(LOC, "Output block (" + std::to_string(j) + "): " + ToString(current_seed), debug_);
        block                 output_block  = ComputeOutputBlock(current_seed, current_control_bit, key);
        std::vector<uint32_t> output_values = CovertVector(output_block, remaining_bit, params_.element_bitsize);
        for (uint32_t k = j; k < j + utils::Pow(2, remaining_bit); k++) {
            outputs[k] = output_values[k - j];
        }
    }
}

block DpfEvaluator::ComputeOutputBlock(const block &final_seed, bool final_control_bit, const DpfKey &key) const {
    // Compute the mask block and remaining bits
    block    mask          = zero_and_all_one[final_control_bit];
    uint32_t remaining_bit = params_.input_bitsize - params_.terminate_bitsize;

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
    uint32_t remaining_bit = params_.input_bitsize - params_.terminate_bitsize;

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
