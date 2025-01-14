#include "dpf_eval.h"

#include <stack>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "prg.h"

namespace fsswm {
namespace fss {
namespace dpf {

using utils::GetLowerNBits;
using utils::Logger;
using utils::Mod;
using utils::Pow;
using utils::TimerManager;

DpfEvaluator::DpfEvaluator(const DpfParameters &params)
    : params_(params),
      G_(prg::PseudoRandomGeneratorSingleton::GetInstance()) {
}

uint32_t DpfEvaluator::EvaluateAt(const DpfKey &key, uint32_t x) const {
    if (!ValidateInput(x)) {
        Logger::FatalLog(LOC, "Invalid input value: x=" + std::to_string(x));
        std::exit(EXIT_FAILURE);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::string evalat_type = (params_.GetEnableEarlyTermination()) ? "optimized" : "naive";
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate input with DPF key(" + evalat_type + " approach)"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id));
    Logger::DebugLog(LOC, "Input: " + std::to_string(x));
#endif

    if (params_.GetEnableEarlyTermination()) {
        return EvaluateAtOptimized(key, x);
    } else {
        return EvaluateAtNaive(key, x);
    }
}

void DpfEvaluator::EvaluateAt(const std::vector<DpfKey> &keys, const std::vector<uint32_t> &x, std::vector<uint32_t> &outputs) const {
    if (keys.size() != x.size()) {
        Logger::FatalLog(LOC, "Number of keys and x values do not match");
        std::exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < keys.size(); ++i) {
        outputs[i] = EvaluateAt(keys[i], x[i]);
    }
}

uint32_t DpfEvaluator::EvaluateAtNaive(const DpfKey &key, uint32_t x) const {
    uint32_t n = params_.GetInputBitsize();
    uint32_t e = params_.GetOutputBitsize();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Evaluate the DPF key
    alignas(16) std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
    std::array<bool, 2>              expanded_control_bits;    // expanded_control_bits[keep or lose]

    for (uint32_t i = 0; i < n; ++i) {
        EvaluateNextSeed(i, seed, control_bit, expanded_seeds, expanded_control_bits, key);

        // Update the seed and control bit based on the current bit
        bool current_bit = (x & (1U << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#if LOG_LEVEL >= LOG_LEVEL_TRACE
        std::string level_str = "|Level=" + std::to_string(i) + "| ";
        Logger::TraceLog(LOC, level_str + "Current bit: " + std::to_string(current_bit));
        Logger::TraceLog(LOC, level_str + "Next seed: " + ToString(seed));
        Logger::TraceLog(LOC, level_str + "Next control bit: " + std::to_string(control_bit));
#endif
    }
    // Compute the final output
    uint32_t output = Pow(-1, key.party_id) * (Convert(seed, e) + (control_bit * Convert(key.output, e)));
    return Mod(output, e);
}

uint32_t DpfEvaluator::EvaluateAtOptimized(const DpfKey &key, uint32_t x) const {
    uint32_t n  = params_.GetInputBitsize();
    uint32_t e  = params_.GetOutputBitsize();
    uint32_t nu = params_.GetTerminateBitsize();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Evaluate the DPF key
    alignas(16) std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
    std::array<bool, 2>              expanded_control_bits;    // expanded_control_bits[keep or lose]

    for (uint32_t i = 0; i < nu; ++i) {
        EvaluateNextSeed(i, seed, control_bit, expanded_seeds, expanded_control_bits, key);

        // Update the seed and control bit based on the current bit
        bool current_bit = (x & (1U << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#if LOG_LEVEL >= LOG_LEVEL_TRACE
        std::string level_str = "|Level=" + std::to_string(i) + "| ";
        Logger::TraceLog(LOC, level_str + "Current bit: " + std::to_string(current_bit));
        Logger::TraceLog(LOC, level_str + "Next seed: " + ToString(seed));
        Logger::TraceLog(LOC, level_str + "Next control bit: " + std::to_string(control_bit));
#endif
    }

    // Compute the final output
    block    output_block = ComputeOutputBlock(seed, control_bit, key);
    uint32_t x_hat        = GetLowerNBits(x, n - nu);
    uint32_t output       = GetValueFromSplitBlock(output_block, n - nu, x_hat);
    return Mod(output, e);
}

bool DpfEvaluator::ValidateInput(const uint32_t x) const {
    bool valid = true;
    if (x >= (1UL << params_.GetInputBitsize())) {
        valid = false;
    }
    return valid;
}

void DpfEvaluator::EvaluateFullDomain(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint32_t num_nodes = 1U << nu;

    // Check if the domain size
    if (params_.GetOutputBitsize() == 1) {
        Logger::FatalLog(LOC, "You should use EvaluateFullDomainOneBit for the domain size of 1 bit");
        std::exit(EXIT_FAILURE);
    }

    // Check output vector size
    if (outputs.size() != num_nodes) {
        outputs.resize(num_nodes);
    }

    // Evaluate the DPF key for all possible x values
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id));
#endif
    switch (fde_type) {
        case EvalType::kNaive:
            Logger::FatalLog(LOC, "Naive approach is not supported for the block output");
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
            Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(fde_type)));
            std::exit(EXIT_FAILURE);
    }
}

void DpfEvaluator::EvaluateFullDomain(const DpfKey &key, std::vector<uint32_t> &outputs) const {
    uint32_t n         = params_.GetInputBitsize();
    uint32_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint32_t num_nodes = 1U << nu;

    // TimerManager tm;
    // int32_t             tm_eval = tm.CreateNewTimer("Eval full domain");
    // int32_t             tm_out  = tm.CreateNewTimer("Compute output values");
    // tm.SelectTimer(tm_eval);
    // tm.Start();

    // Check if the domain size
    if (params_.GetOutputBitsize() == 1) {
        Logger::FatalLog(LOC, "You should use EvaluateFullDomainOneBit for the domain size of 1 bit");
        std::exit(EXIT_FAILURE);
    }

    // Check output vector size
    if (outputs.size() != 1U << params_.GetInputBitsize()) {
        outputs.resize(1U << params_.GetInputBitsize());
    }
    std::vector<block> outputs_block(num_nodes, zero_block);

    // Evaluate the DPF key for all possible x values
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id));
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
            Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(fde_type)));
            std::exit(EXIT_FAILURE);
    }
    // tm.Stop();
    // tm.SelectTimer(tm_out);
    // tm.Start();
    if (fde_type != EvalType::kNaive) {
        ConvertVector(outputs_block, n - nu, params_.GetOutputBitsize(), outputs);
    }
    // tm.Stop("Compute output values");
    // tm.PrintAllResults(TimeUnit::MICROSECONDS);
}

void DpfEvaluator::EvaluateFullDomainTwoKeys(const DpfKey &key1, const DpfKey &key2, std::vector<uint32_t> &out1, std::vector<uint32_t> &out2) const {
    uint32_t n         = params_.GetInputBitsize();
    uint32_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint32_t num_nodes = 1U << nu;

    // Check if the domain size
    if (params_.GetOutputBitsize() == 1) {
        Logger::FatalLog(LOC, "You should use EvaluateFullDomainOneBit for the domain size of 1 bit");
        std::exit(EXIT_FAILURE);
    }

    // Check output vector size
    if (out1.size() != 1U << params_.GetInputBitsize()) {
        out1.resize(1U << params_.GetInputBitsize());
    }
    if (out2.size() != 1U << params_.GetInputBitsize()) {
        out2.resize(1U << params_.GetInputBitsize());
    }
    std::vector<block> out1_block(num_nodes, zero_block);
    std::vector<block> out2_block(num_nodes, zero_block);

    // Evaluate the DPF key for all possible x values
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(key1.party_id));
#endif
    switch (fde_type) {
        case EvalType::kIterSingleBatch_2Keys:
            FullDomainIterativeSingleBatchTwoKeys(key1, key2, out1_block, out2_block);
            break;
        default:
            Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(fde_type)));
            std::exit(EXIT_FAILURE);
    }
    ConvertVector(out1_block, out2_block, n - nu, params_.GetOutputBitsize(), out1, out2);
}

void DpfEvaluator::EvaluateFullDomainOneBit(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint32_t num_nodes = 1U << nu;

    // Check if the domain size
    if (params_.GetOutputBitsize() != 1) {
        Logger::FatalLog(LOC, "This function is only for the domain size of 1 bit");
        std::exit(EXIT_FAILURE);
    }

    // Check output vector size
    if (outputs.size() != num_nodes) {
        outputs.resize(num_nodes);
    }

    // Evaluate the DPF key for all possible x values
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(key.party_id));
#endif
    switch (fde_type) {
        case EvalType::kNaive:
            Logger::FatalLog(LOC, "Naive approach is not supported for the domain size of 1 bit");
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
            Logger::FatalLog(LOC, "Invalid evaluation type: " + std::to_string(static_cast<int>(fde_type)));
            std::exit(EXIT_FAILURE);
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

#if LOG_LEVEL >= LOG_LEVEL_TRACE
    std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
    Logger::TraceLog(LOC, level_str + "Current seed: " + ToString(current_seed));
    Logger::TraceLog(LOC, level_str + "Current control bit: " + std::to_string(current_control_bit));
    Logger::TraceLog(LOC, level_str + "Expanded seed (L): " + ToString(expanded_seeds[kLeft]));
    Logger::TraceLog(LOC, level_str + "Expanded seed (R): " + ToString(expanded_seeds[kRight]));
    Logger::TraceLog(LOC, level_str + "Expanded control bit (L, R): " + std::to_string(expanded_control_bits[kLeft]) + ", " + std::to_string(expanded_control_bits[kRight]));
#endif

    // Apply correction word if control bit is true
    const block mask       = key.cw_seed[current_level] & zero_and_all_one[current_control_bit];
    expanded_seeds[kLeft]  = _mm_xor_si128(expanded_seeds[kLeft], mask);
    expanded_seeds[kRight] = _mm_xor_si128(expanded_seeds[kRight], mask);

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
    uint32_t nu            = params_.GetTerminateBitsize();
    uint32_t remaining_bit = params_.GetInputBitsize() - nu;

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = nu;
    uint32_t last_idx      = 1U << last_depth;

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

#if LOG_LEVEL >= LOG_LEVEL_TRACE
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            Logger::TraceLog(LOC, level_str + "Current bit: " + std::to_string(current_bit));
            Logger::TraceLog(LOC, level_str + "Current seed: " + ToString(prev_seeds[current_level]));
            Logger::TraceLog(LOC, level_str + "Current control bit: " + std::to_string(prev_control_bits[current_level]));
            Logger::TraceLog(LOC, level_str + "Expanded seed: " + ToString(expanded_seed));
            Logger::TraceLog(LOC, level_str + "Expanded control bit: " + std::to_string(expanded_control_bit));
#endif

            // Apply correction word if control bit is true
            bool cw_control_bit = current_bit ? key.cw_control_right[current_level] : key.cw_control_left[current_level];
            expanded_seed       = _mm_xor_si128(expanded_seed, _mm_and_si128(key.cw_seed[current_level], zero_and_all_one[prev_control_bits[current_level]]));
            expanded_control_bit ^= (cw_control_bit & prev_control_bits[current_level]);

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            prev_seeds[current_level]        = expanded_seed;
            prev_control_bits[current_level] = expanded_control_bit;
        }

        if (remaining_bit == 2) {
            if (key.party_id) {
                outputs[current_idx] = _mm_sub_epi32(zero_block, _mm_add_epi32(prev_seeds[current_level], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level]], key.output)));
            } else {
                outputs[current_idx] = _mm_add_epi32(prev_seeds[current_level], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level]], key.output));
            }
        } else if (remaining_bit == 3) {
            if (key.party_id) {
                outputs[current_idx] = _mm_sub_epi16(zero_block, _mm_add_epi16(prev_seeds[current_level], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level]], key.output)));
            } else {
                outputs[current_idx] = _mm_add_epi16(prev_seeds[current_level], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level]], key.output));
            }
        } else if (remaining_bit == 7) {
            outputs[current_idx] = _mm_xor_si128(prev_seeds[current_level], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level]], key.output));
        } else {
            Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            std::exit(EXIT_FAILURE);
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < outputs.size(); ++i) {
        Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(outputs[i]));
    }
#endif
}

void DpfEvaluator::FullDomainIterativeDouble(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu            = params_.GetTerminateBitsize();
    uint32_t remaining_bit = params_.GetInputBitsize() - nu;

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = nu;
    uint32_t last_idx      = 1U << last_depth;

    // Store the expanded seeds and control bits
    alignas(16) std::array<block, 2> expanded_seeds;
    std::array<bool, 2>              expanded_control_bits;
    std::stack<block>                seed_stack;
    std::stack<bool>                 control_bit_stack;

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

        if (remaining_bit == 2) {
            if (key.party_id) {
                outputs[current_idx + 0] = _mm_sub_epi32(zero_block, _mm_add_epi32(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output)));
                seed_stack.pop();
                control_bit_stack.pop();
                outputs[current_idx + 1] = _mm_sub_epi32(zero_block, _mm_add_epi32(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output)));
                seed_stack.pop();
                control_bit_stack.pop();
            } else {
                outputs[current_idx + 0] = _mm_add_epi32(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output));
                seed_stack.pop();
                control_bit_stack.pop();
                outputs[current_idx + 1] = _mm_add_epi32(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output));
                seed_stack.pop();
                control_bit_stack.pop();
            }
        } else if (remaining_bit == 3) {
            if (key.party_id) {
                outputs[current_idx + 0] = _mm_sub_epi16(zero_block, _mm_add_epi16(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output)));
                seed_stack.pop();
                control_bit_stack.pop();
                outputs[current_idx + 1] = _mm_sub_epi16(zero_block, _mm_add_epi16(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output)));
                seed_stack.pop();
                control_bit_stack.pop();
            } else {
                outputs[current_idx] = _mm_add_epi16(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output));
                seed_stack.pop();
                control_bit_stack.pop();
                outputs[current_idx + 1] = _mm_add_epi16(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output));
                seed_stack.pop();
                control_bit_stack.pop();
            }
        } else if (remaining_bit == 7) {
            outputs[current_idx] = _mm_xor_si128(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output));
            seed_stack.pop();
            control_bit_stack.pop();
            outputs[current_idx + 1] = _mm_xor_si128(seed_stack.top(), _mm_and_si128(zero_and_all_one[control_bit_stack.top()], key.output));
            seed_stack.pop();
            control_bit_stack.pop();
        } else {
            Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            std::exit(EXIT_FAILURE);
        }

        // Update the current index
        current_idx += 2;
        int shift = (current_idx - 1U) ^ current_idx;
        current_level -= log2floor(shift);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < outputs.size(); ++i) {
        Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(outputs[i]));
    }
#endif
}

void DpfEvaluator::FullDomainIterativeSingleBatch(const DpfKey &key, std::vector<block> &outputs) const {
    uint32_t nu            = params_.GetTerminateBitsize();
    uint32_t remaining_bit = params_.GetInputBitsize() - nu;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint32_t i = 0; i < 3; ++i) {
        alignas(16) std::array<block, 2> expanded_seeds;
        std::array<bool, 2>              expanded_control_bits;
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
    for (uint32_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint32_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint32_t i = 0; i < 8; ++i) {
                expanded_control_bits[i] = getLSB(expanded_seeds[i]);
            }

#if LOG_LEVEL >= LOG_LEVEL_TRACE
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 8; ++i) {
                Logger::TraceLog(LOC, level_str + "Current bit: " + std::to_string(current_bit));
                Logger::TraceLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(prev_seeds[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(prev_control_bits[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Expanded seed (" + std::to_string(i) + "): " + ToString(expanded_seeds[i]));
                Logger::TraceLog(LOC, level_str + "Expanded control bit (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i]));
            }
#endif

            // Apply correction word if control bit is true
            bool  cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            block cw_seed        = key.cw_seed[current_level + 3];
            expanded_seeds[0]    = _mm_xor_si128(expanded_seeds[0], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][0]]));
            expanded_seeds[1]    = _mm_xor_si128(expanded_seeds[1], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][1]]));
            expanded_seeds[2]    = _mm_xor_si128(expanded_seeds[2], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][2]]));
            expanded_seeds[3]    = _mm_xor_si128(expanded_seeds[3], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][3]]));
            expanded_seeds[4]    = _mm_xor_si128(expanded_seeds[4], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][4]]));
            expanded_seeds[5]    = _mm_xor_si128(expanded_seeds[5], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][5]]));
            expanded_seeds[6]    = _mm_xor_si128(expanded_seeds[6], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][6]]));
            expanded_seeds[7]    = _mm_xor_si128(expanded_seeds[7], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][7]]));

            for (uint32_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint32_t i = 0; i < 8; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        if (remaining_bit == 2) {
            if (key.party_id) {
                for (uint32_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_sub_epi32(zero_block, _mm_add_epi32(prev_seeds[current_level][i], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level][i]], key.output)));
                }
            } else {
                for (uint32_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_add_epi32(prev_seeds[current_level][i], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level][i]], key.output));
                }
            }
        } else if (remaining_bit == 3) {
            if (key.party_id) {
                for (uint32_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_sub_epi16(zero_block, _mm_add_epi16(prev_seeds[current_level][i], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level][i]], key.output)));
                }
            } else {
                for (uint32_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_add_epi16(prev_seeds[current_level][i], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level][i]], key.output));
                }
            }
        } else if (remaining_bit == 7) {
            for (uint32_t i = 0; i < 8; ++i) {
                outputs[i * last_idx + current_idx] = _mm_xor_si128(prev_seeds[current_level][i], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level][i]], key.output));
            }
        } else {
            Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            std::exit(EXIT_FAILURE);
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < outputs.size(); ++i) {
        Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(outputs[i]));
    }
#endif
}

void DpfEvaluator::FullDomainIterativeSingleBatchTwoKeys(const DpfKey &key1, const DpfKey &key2, std::vector<block> &out1, std::vector<block> &out2) const {
    uint32_t nu            = params_.GetTerminateBitsize();
    uint32_t remaining_bit = params_.GetInputBitsize() - nu;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds1{key1.init_seed}, start_seeds2{key2.init_seed}, next_seeds1, next_seeds2;
    std::vector<bool>  start_control_bits1{key1.party_id != 0}, start_control_bits2{key2.party_id != 0}, next_control_bits1, next_control_bits2;

    for (uint32_t i = 0; i < 3; ++i) {
        alignas(16) std::array<block, 2> expanded_seeds1, expanded_seeds2;
        std::array<bool, 2>              expanded_control_bits1, expanded_control_bits2;
        next_seeds1.resize(1U << (i + 1));
        next_control_bits1.resize(1U << (i + 1));
        next_seeds2.resize(1U << (i + 1));
        next_control_bits2.resize(1U << (i + 1));
        for (size_t j = 0; j < start_seeds1.size(); j++) {
            EvaluateNextSeed(i, start_seeds1[j], start_control_bits1[j], expanded_seeds1, expanded_control_bits1, key1);
            EvaluateNextSeed(i, start_seeds2[j], start_control_bits2[j], expanded_seeds2, expanded_control_bits2, key2);
            next_seeds1[j * 2]            = expanded_seeds1[kLeft];
            next_seeds1[j * 2 + 1]        = expanded_seeds1[kRight];
            next_control_bits1[j * 2]     = expanded_control_bits1[kLeft];
            next_control_bits1[j * 2 + 1] = expanded_control_bits1[kRight];
            next_seeds2[j * 2]            = expanded_seeds2[kLeft];
            next_seeds2[j * 2 + 1]        = expanded_seeds2[kRight];
            next_control_bits2[j * 2]     = expanded_control_bits2[kLeft];
            next_control_bits2[j * 2 + 1] = expanded_control_bits2[kRight];
        }
        start_seeds1        = std::move(next_seeds1);
        start_control_bits1 = std::move(next_control_bits1);
        start_seeds2        = std::move(next_seeds2);
        start_control_bits2 = std::move(next_control_bits2);
    }

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint32_t last_idx      = 1U << last_depth;

    // Store the seeds and control bits
    std::array<block, 16>              expanded_seeds;
    std::array<bool, 16>               expanded_control_bits;
    std::vector<std::array<block, 16>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 16>>  prev_control_bits(last_depth + 1);

    // Evaluate the DPF key
    for (uint32_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]            = start_seeds1[i];
        prev_control_bits[0][i]     = start_control_bits1[i];
        prev_seeds[0][i + 8]        = start_seeds2[i];
        prev_control_bits[0][i + 8] = start_control_bits2[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint32_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint32_t i = 0; i < 16; ++i) {
                expanded_control_bits[i] = getLSB(expanded_seeds[i]);
            }

#if LOG_LEVEL >= LOG_LEVEL_TRACE
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 16; ++i) {
                Logger::TraceLog(LOC, level_str + "Current bit: " + std::to_string(current_bit));
                Logger::TraceLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(prev_seeds[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(prev_control_bits[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Expanded seed (" + std::to_string(i) + "): " + ToString(expanded_seeds[i]));
                Logger::TraceLog(LOC, level_str + "Expanded control bit (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i]));
            }
#endif

            // Apply correction word if control bit is true
            bool  cw_control_bit1 = current_bit ? key1.cw_control_right[current_level + 3] : key1.cw_control_left[current_level + 3];
            bool  cw_control_bit2 = current_bit ? key2.cw_control_right[current_level + 3] : key2.cw_control_left[current_level + 3];
            block cw_seed1        = key1.cw_seed[current_level + 3];
            block cw_seed2        = key2.cw_seed[current_level + 3];
            expanded_seeds[0]     = _mm_xor_si128(expanded_seeds[0], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][0]]));
            expanded_seeds[1]     = _mm_xor_si128(expanded_seeds[1], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][1]]));
            expanded_seeds[2]     = _mm_xor_si128(expanded_seeds[2], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][2]]));
            expanded_seeds[3]     = _mm_xor_si128(expanded_seeds[3], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][3]]));
            expanded_seeds[4]     = _mm_xor_si128(expanded_seeds[4], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][4]]));
            expanded_seeds[5]     = _mm_xor_si128(expanded_seeds[5], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][5]]));
            expanded_seeds[6]     = _mm_xor_si128(expanded_seeds[6], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][6]]));
            expanded_seeds[7]     = _mm_xor_si128(expanded_seeds[7], _mm_and_si128(cw_seed1, zero_and_all_one[prev_control_bits[current_level][7]]));
            expanded_seeds[8]     = _mm_xor_si128(expanded_seeds[8], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][8]]));
            expanded_seeds[9]     = _mm_xor_si128(expanded_seeds[9], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][9]]));
            expanded_seeds[10]    = _mm_xor_si128(expanded_seeds[10], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][10]]));
            expanded_seeds[11]    = _mm_xor_si128(expanded_seeds[11], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][11]]));
            expanded_seeds[12]    = _mm_xor_si128(expanded_seeds[12], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][12]]));
            expanded_seeds[13]    = _mm_xor_si128(expanded_seeds[13], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][13]]));
            expanded_seeds[14]    = _mm_xor_si128(expanded_seeds[14], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][14]]));
            expanded_seeds[15]    = _mm_xor_si128(expanded_seeds[15], _mm_and_si128(cw_seed2, zero_and_all_one[prev_control_bits[current_level][15]]));

            for (uint32_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit1 & prev_control_bits[current_level][i]);
                expanded_control_bits[i + 8] ^= (cw_control_bit2 & prev_control_bits[current_level][i + 8]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint32_t i = 0; i < 16; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        if (remaining_bit == 2) {
            if (key1.party_id) {
                for (uint32_t i = 0; i < 8; ++i) {
                    uint32_t access_idx = i * last_idx + current_idx;
                    out1[access_idx]    = _mm_sub_epi32(zero_block, _mm_add_epi32(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key1.output));
                    out2[access_idx]    = _mm_sub_epi32(zero_block, _mm_add_epi32(prev_seeds[current_level][i + 8], zero_and_all_one[prev_control_bits[current_level][i + 8]] & key2.output));
                }
            } else {
                for (uint32_t i = 0; i < 8; ++i) {
                    uint32_t access_idx = i * last_idx + current_idx;
                    out1[access_idx]    = _mm_add_epi32(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key1.output);
                    out2[access_idx]    = _mm_add_epi32(prev_seeds[current_level][i + 8], zero_and_all_one[prev_control_bits[current_level][i + 8]] & key2.output);
                }
            }
        } else if (remaining_bit == 3) {
            if (key1.party_id) {
                for (uint32_t i = 0; i < 8; ++i) {
                    uint32_t access_idx = i * last_idx + current_idx;
                    out1[access_idx]    = _mm_sub_epi16(zero_block, _mm_add_epi16(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key1.output));
                    out2[access_idx]    = _mm_sub_epi16(zero_block, _mm_add_epi16(prev_seeds[current_level][i + 8], zero_and_all_one[prev_control_bits[current_level][i + 8]] & key2.output));
                }
            } else {
                for (uint32_t i = 0; i < 8; ++i) {
                    uint32_t access_idx = i * last_idx + current_idx;
                    out1[access_idx]    = _mm_add_epi16(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key1.output);
                    out2[access_idx]    = _mm_add_epi16(prev_seeds[current_level][i + 8], zero_and_all_one[prev_control_bits[current_level][i + 8]] & key2.output);
                }
            }
        } else if (remaining_bit == 7) {
            for (uint32_t i = 0; i < 8; ++i) {
                uint32_t access_idx = i * last_idx + current_idx;
                out1[access_idx]    = prev_seeds[current_level][i] ^ (zero_and_all_one[prev_control_bits[current_level][i]] & key1.output);
                out2[access_idx]    = prev_seeds[current_level][i + 8] ^ (zero_and_all_one[prev_control_bits[current_level][i + 8]] & key2.output);
            }
        } else {
            Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            std::exit(EXIT_FAILURE);
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < out1.size(); ++i) {
        Logger::DebugLog(LOC, "Output1 seed (" + std::to_string(i) + "): " + ToString(out1[i]));
    }
    for (uint32_t i = 0; i < out2.size(); ++i) {
        Logger::DebugLog(LOC, "Output2 seed (" + std::to_string(i) + "): " + ToString(out2[i]));
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

    for (uint32_t i = 0; i < 3; ++i) {
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
            for (uint32_t i = 0; i < 8; ++i) {
                expanded_control_bits[kLeft][i]  = getLSB(expanded_seeds[kLeft][i]);
                expanded_control_bits[kRight][i] = getLSB(expanded_seeds[kRight][i]);
            }

#if LOG_LEVEL >= LOG_LEVEL_TRACE
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 8; ++i) {
                Logger::TraceLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(current_seeds[i]));
                Logger::TraceLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(current_control_bits[i]));
                Logger::TraceLog(LOC, level_str + "Expanded seed (L) (" + std::to_string(i) + "): " + ToString(expanded_seeds[kLeft][i]));
                Logger::TraceLog(LOC, level_str + "Expanded seed (R) (" + std::to_string(i) + "): " + ToString(expanded_seeds[kRight][i]));
                Logger::TraceLog(LOC, level_str + "Expanded control bit (L, R) (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i][kLeft]) + ", " + std::to_string(expanded_control_bits[i][kRight]));
            }
#endif

            // Apply correction word if control bit is true
            for (uint32_t i = 0; i < 8; ++i) {
                expanded_seeds[kLeft][i]  = _mm_xor_si128(expanded_seeds[kLeft][i], _mm_and_si128(key.cw_seed[current_level + 3], zero_and_all_one[current_control_bits[i]]));
                expanded_seeds[kRight][i] = _mm_xor_si128(expanded_seeds[kRight][i], _mm_and_si128(key.cw_seed[current_level + 3], zero_and_all_one[current_control_bits[i]]));
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
            for (uint32_t i = 0; i < 2; ++i) {
                auto &tmp_seeds        = seed_stacks.top();
                auto &tmp_control_bits = control_bit_stacks.top();
                if (key.party_id) {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_sub_epi32(zero_block, _mm_add_epi32(tmp_seeds[j], _mm_and_si128(zero_and_all_one[tmp_control_bits[j]], key.output)));
                    }
                } else {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_add_epi32(tmp_seeds[j], _mm_and_si128(zero_and_all_one[tmp_control_bits[j]], key.output));
                    }
                }
                seed_stacks.pop();
                control_bit_stacks.pop();
            }
        } else if (remaining_bit == 3) {
            for (uint32_t i = 0; i < 2; ++i) {
                auto &tmp_seeds        = seed_stacks.top();
                auto &tmp_control_bits = control_bit_stacks.top();
                if (key.party_id) {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_sub_epi16(zero_block, _mm_add_epi16(tmp_seeds[j], _mm_and_si128(zero_and_all_one[tmp_control_bits[j]], key.output)));
                    }
                } else {
                    for (uint32_t j = 0; j < 8; j++) {
                        outputs[j * last_idx + current_idx + i] = _mm_add_epi16(tmp_seeds[j], _mm_and_si128(zero_and_all_one[tmp_control_bits[j]], key.output));
                    }
                }
                seed_stacks.pop();
                control_bit_stacks.pop();
            }
        } else if (remaining_bit == 7) {
            for (uint32_t i = 0; i < 2; ++i) {
                auto &tmp_seeds        = seed_stacks.top();
                auto &tmp_control_bits = control_bit_stacks.top();
                for (uint32_t j = 0; j < 8; j++) {
                    outputs[j * last_idx + current_idx + i] = _mm_xor_si128(tmp_seeds[j], _mm_and_si128(zero_and_all_one[tmp_control_bits[j]], key.output));
                }
                seed_stacks.pop();
                control_bit_stacks.pop();
            }
        } else {
            Logger::FatalLog(LOC, "Invalid remaining bit: " + std::to_string(remaining_bit));
            std::exit(EXIT_FAILURE);
        }

        // Update the current index
        current_idx += 2;
        int shift = (current_idx - 1U) ^ current_idx;
        current_level -= log2floor(shift);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < num_nodes; ++i) {
        Logger::DebugLog(LOC, "Output seed (" + std::to_string(i) + "): " + ToString(outputs[i]));
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
        std::array<bool, 2>              expanded_control_bits;    // expanded_control_bits[keep or lose]

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
            output = _mm_sub_epi32(zero_block, _mm_add_epi32(final_seed, _mm_and_si128(mask, key.output)));
        } else {
            output = _mm_add_epi32(final_seed, _mm_and_si128(mask, key.output));
        }
    } else if (remaining_bit == 3) {
        // Reduce 3 levels (2^3=8 nodes) of the tree (Additive share)
        if (key.party_id) {
            output = _mm_sub_epi16(zero_block, _mm_add_epi16(final_seed, _mm_and_si128(mask, key.output)));
        } else {
            output = _mm_add_epi16(final_seed, _mm_and_si128(mask, key.output));
        }
    } else if (remaining_bit == 7) {
        // Reduce 7 levels (2^7=128 nodes) of the tree (Additive share)
        output = _mm_xor_si128(final_seed, _mm_and_si128(mask, key.output));
    } else {
        Logger::FatalLog(LOC, "Unsupported termination bitsize: " + std::to_string(remaining_bit));
        std::exit(EXIT_FAILURE);
    }
    return output;
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
