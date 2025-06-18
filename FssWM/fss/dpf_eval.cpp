#include "dpf_eval.h"

#include <stack>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "prg.h"

namespace fsswm {
namespace fss {
namespace dpf {

DpfEvaluator::DpfEvaluator(const DpfParameters &params)
    : params_(params),
      G_(prg::PseudoRandomGenerator::GetInstance()) {
}

uint64_t DpfEvaluator::EvaluateAt(const DpfKey &key, uint64_t x) const {
    if (!ValidateInput(x)) {
        Logger::FatalLog(LOC, "Invalid input value: x=" + ToString(x));
        std::exit(EXIT_FAILURE);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::string eval_type = (params_.GetEnableEarlyTermination()) ? "optimized" : "naive";
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate input with DPF key(" + eval_type + " approach)"));
    Logger::DebugLog(LOC, "Party ID: " + ToString(key.party_id));
    Logger::DebugLog(LOC, "Input: " + ToString(x));
#endif

    if (params_.GetEnableEarlyTermination()) {
        return EvaluateAtOptimized(key, x);
    } else {
        return EvaluateAtNaive(key, x);
    }
}

void DpfEvaluator::EvaluateAt(const std::vector<DpfKey> &keys, const std::vector<uint64_t> &x, std::vector<uint64_t> &outputs) const {
    if (keys.size() != x.size()) {
        Logger::FatalLog(LOC, "Number of keys and x values do not match");
        std::exit(EXIT_FAILURE);
    }

    for (uint64_t i = 0; i < keys.size(); ++i) {
        outputs[i] = EvaluateAt(keys[i], x[i]);
    }
}

uint64_t DpfEvaluator::EvaluateAtNaive(const DpfKey &key, uint64_t x) const {
    uint64_t n = params_.GetInputBitsize();
    uint64_t e = params_.GetOutputBitsize();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Evaluate the DPF key
    std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
    std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

    for (uint64_t i = 0; i < n; ++i) {
        EvaluateNextSeed(i, seed, control_bit, expanded_seeds, expanded_control_bits, key);

        // Update the seed and control bit based on the current bit
        bool current_bit = (x & (1U << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#if LOG_LEVEL >= LOG_LEVEL_TRACE
        std::string level_str = "|Level=" + ToString(i) + "| ";
        Logger::TraceLog(LOC, level_str + "Current bit: " + ToString(current_bit));
        Logger::TraceLog(LOC, level_str + "Next seed: " + Format(seed));
        Logger::TraceLog(LOC, level_str + "Next control bit: " + ToString(control_bit));
#endif
    }
    // Compute the final output
    G_.Expand(seed, seed, true);
    uint64_t output = Sign(key.party_id) * (Convert(seed, e) + (control_bit * Convert(key.output, e)));
    return Mod(output, e);
}

uint64_t DpfEvaluator::EvaluateAtOptimized(const DpfKey &key, uint64_t x) const {
    uint64_t   n    = params_.GetInputBitsize();
    uint64_t   e    = params_.GetOutputBitsize();
    uint64_t   nu   = params_.GetTerminateBitsize();
    OutputType mode = params_.GetOutputType();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    // Evaluate the DPF key
    std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
    std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

    for (uint64_t i = 0; i < nu; ++i) {
        EvaluateNextSeed(i, seed, control_bit, expanded_seeds, expanded_control_bits, key);

        // Update the seed and control bit based on the current bit
        bool current_bit = (x & (1U << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#if LOG_LEVEL >= LOG_LEVEL_TRACE
        std::string level_str = "|Level=" + ToString(i) + "| ";
        Logger::TraceLog(LOC, level_str + "Current bit: " + ToString(current_bit));
        Logger::TraceLog(LOC, level_str + "Next seed: " + Format(seed));
        Logger::TraceLog(LOC, level_str + "Next control bit: " + ToString(control_bit));
#endif
    }

    // Compute the final output
    block    output_block = ComputeOutputBlock(seed, control_bit, key);
    uint64_t x_hat        = GetLowerNBits(x, n - nu);
    uint64_t output       = GetSplitBlockValue(output_block, n - nu, x_hat, mode);
    return Mod(output, e);
}

bool DpfEvaluator::ValidateInput(const uint64_t x) const {
    bool valid = true;
    if (x >= (1UL << params_.GetInputBitsize())) {
        valid = false;
    }
    return valid;
}

void DpfEvaluator::EvaluateFullDomain(const DpfKey &key, std::vector<block> &outputs) const {
    uint64_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint64_t num_nodes = 1U << nu;

    // Check output vector size
    if (outputs.size() != num_nodes) {
        Logger::FatalLog(LOC, "Output vector size does not match the number of nodes: " + ToString(num_nodes));
        std::exit(EXIT_FAILURE);
    }

    // Evaluate the DPF key for all possible x values
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)));
    Logger::DebugLog(LOC, "Party ID: " + ToString(key.party_id));
#endif
    switch (fde_type) {
        case EvalType::kNaive:
            Logger::FatalLog(LOC, "Naive approach is not supported for the block output");
            break;
        case EvalType::kRecursion:
            FullDomainRecursion(key, outputs);
            break;
        case EvalType::kIterSingleBatch:
            FullDomainIterativeSingleBatch(key, outputs);
            break;
        default:
            Logger::FatalLog(LOC, "Invalid evaluation type: " + ToString(static_cast<int>(fde_type)));
            std::exit(EXIT_FAILURE);
    }
}

void DpfEvaluator::EvaluateFullDomain(const DpfKey &key, std::vector<uint64_t> &outputs) const {
    uint64_t n         = params_.GetInputBitsize();
    uint64_t nu        = params_.GetTerminateBitsize();
    EvalType fde_type  = params_.GetFdeEvalType();
    uint64_t num_nodes = 1U << nu;

    // Check output vector size
    if (outputs.size() != 1U << params_.GetInputBitsize()) {
        Logger::FatalLog(LOC, "Output vector size does not match the number of nodes: " + ToString(num_nodes));
        std::exit(EXIT_FAILURE);
    }
    std::vector<block> outputs_block(num_nodes);

    // Evaluate the DPF key for all possible x values
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate full domain " + GetEvalTypeString(fde_type)));
    Logger::DebugLog(LOC, "Party ID: " + ToString(key.party_id));
#endif
    switch (fde_type) {
        case EvalType::kNaive:
            FullDomainNaive(key, outputs);
            break;
        case EvalType::kRecursion:
            FullDomainRecursion(key, outputs_block);
            break;
        case EvalType::kIterSingleBatch:
            FullDomainIterativeSingleBatch(key, outputs_block);
            break;
        default:
            Logger::FatalLog(LOC, "Invalid evaluation type: " + ToString(static_cast<int>(fde_type)));
            std::exit(EXIT_FAILURE);
    }
    if (fde_type != EvalType::kNaive) {
        SplitBlockToFieldVector(outputs_block, n - nu, params_.GetOutputBitsize(), outputs);
    }
}

void DpfEvaluator::EvaluateNextSeed(
    const uint64_t current_level, const block &current_seed, const bool &current_control_bit,
    std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
    const DpfKey &key) const {
    // Expand the seed and control bits
    G_.DoubleExpand(current_seed, expanded_seeds);
    expanded_control_bits[kLeft]  = GetLsb(expanded_seeds[kLeft]);
    expanded_control_bits[kRight] = GetLsb(expanded_seeds[kRight]);
    SetLsbZero(expanded_seeds[kLeft]);
    SetLsbZero(expanded_seeds[kRight]);

#if LOG_LEVEL >= LOG_LEVEL_TRACE
    std::string level_str = "|Level=" + ToString(current_level) + "| ";
    Logger::TraceLog(LOC, level_str + "Current seed: " + Format(current_seed));
    Logger::TraceLog(LOC, level_str + "Current control bit: " + ToString(current_control_bit));
    Logger::TraceLog(LOC, level_str + "Expanded seed (L): " + Format(expanded_seeds[kLeft]));
    Logger::TraceLog(LOC, level_str + "Expanded seed (R): " + Format(expanded_seeds[kRight]));
    Logger::TraceLog(LOC, level_str + "Expanded control bit (L, R): " + ToString(expanded_control_bits[kLeft]) + ", " + ToString(expanded_control_bits[kRight]));
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

block DpfEvaluator::ComputeDotProductBlockSIMD(const DpfKey &key, std::vector<block> &database) const {
    uint64_t nu = params_.GetTerminateBitsize();

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint64_t i = 0; i < 3; ++i) {
        std::array<block, 2> expanded_seeds;
        std::array<bool, 2>  expanded_control_bits;
        next_seeds.resize(1U << (i + 1));
        next_control_bits.resize(1U << (i + 1));
        for (size_t j = 0; j < start_seeds.size(); j++) {
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[fss::kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[fss::kRight];
            next_control_bits[j * 2]     = expanded_control_bits[fss::kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[fss::kRight];
        }
        start_seeds        = std::move(next_seeds);
        start_control_bits = std::move(next_control_bits);
    }

    // Initialize the variables
    uint64_t current_level = 0;
    uint64_t current_idx   = 0;
    uint64_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint64_t last_idx      = 1U << last_depth;

    // Store the seeds and control bits
    std::array<block, 8>              expanded_seeds, output_seeds;
    std::array<block, 8>              sums = {zero_block, zero_block, zero_block, zero_block,
                                              zero_block, zero_block, zero_block, zero_block};
    std::array<bool, 8>               expanded_control_bits;
    std::vector<std::array<block, 8>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 8>>  prev_control_bits(last_depth + 1);
    std::vector<block>                byte_expanded_seeds(64);

    // Evaluate the DPF key
    for (uint64_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint64_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint64_t i = 0; i < 8; ++i) {
                expanded_control_bits[i] = GetLsb(expanded_seeds[i]);
                SetLsbZero(expanded_seeds[i]);
            }

            // Apply correction word if control bit is true
            bool  cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            block cw_seed        = key.cw_seed[current_level + 3];
            expanded_seeds[0] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][0]]);
            expanded_seeds[1] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][1]]);
            expanded_seeds[2] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][2]]);
            expanded_seeds[3] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][3]]);
            expanded_seeds[4] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][4]]);
            expanded_seeds[5] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][5]]);
            expanded_seeds[6] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][6]]);
            expanded_seeds[7] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][7]]);

            for (uint64_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint64_t i = 0; i < 8; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        // Seed expansion for the final output
        G_.Expand(prev_seeds[current_level], prev_seeds[current_level], true);

        for (uint64_t j = 0; j < 8; ++j) {
            output_seeds[j] = prev_seeds[current_level][j] ^ (zero_and_all_one[prev_control_bits[current_level][j]] & key.output);
        }

        auto dest = byte_expanded_seeds.data();

        for (uint64_t i = 0; i < 8; ++i) {
            dest[0] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(0);
            dest[1] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(1);
            dest[2] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(2);
            dest[3] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(3);
            dest[4] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(4);
            dest[5] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(5);
            dest[6] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(6);
            dest[7] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(7);

            dest += 8;
        }

        // for (uint64_t i = 0; i < 8; ++i) {
        //     Logger::WarnLog(LOC, "output_seeds[" + ToString(i) + "]=" + Format(output_seeds[i], FormatType::kBin));
        // }

        uint8_t *seed_byte0 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 0;
        uint8_t *seed_byte1 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 1;
        uint8_t *seed_byte2 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 2;
        uint8_t *seed_byte3 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 3;
        uint8_t *seed_byte4 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 4;
        uint8_t *seed_byte5 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 5;
        uint8_t *seed_byte6 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 6;
        uint8_t *seed_byte7 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 7;

        auto input_iter0 = database.data() + ((0UL << current_level) + current_idx) * 128;
        auto input_iter1 = database.data() + ((1UL << current_level) + current_idx) * 128;
        auto input_iter2 = database.data() + ((2UL << current_level) + current_idx) * 128;
        auto input_iter3 = database.data() + ((3UL << current_level) + current_idx) * 128;
        auto input_iter4 = database.data() + ((4UL << current_level) + current_idx) * 128;
        auto input_iter5 = database.data() + ((5UL << current_level) + current_idx) * 128;
        auto input_iter6 = database.data() + ((6UL << current_level) + current_idx) * 128;
        auto input_iter7 = database.data() + ((7UL << current_level) + current_idx) * 128;

        // Calculate the dot product
        for (uint64_t j = 0; j < 128; ++j) {
            auto input0 = input_iter0[j] & zero_and_all_one[seed_byte0[j]];
            auto input1 = input_iter1[j] & zero_and_all_one[seed_byte1[j]];
            auto input2 = input_iter2[j] & zero_and_all_one[seed_byte2[j]];
            auto input3 = input_iter3[j] & zero_and_all_one[seed_byte3[j]];
            auto input4 = input_iter4[j] & zero_and_all_one[seed_byte4[j]];
            auto input5 = input_iter5[j] & zero_and_all_one[seed_byte5[j]];
            auto input6 = input_iter6[j] & zero_and_all_one[seed_byte6[j]];
            auto input7 = input_iter7[j] & zero_and_all_one[seed_byte7[j]];

            sums[0] = sums[0] ^ input0;
            sums[1] = sums[1] ^ input1;
            sums[2] = sums[2] ^ input2;
            sums[3] = sums[3] ^ input3;
            sums[4] = sums[4] ^ input4;
            sums[5] = sums[5] ^ input5;
            sums[6] = sums[6] ^ input6;
            sums[7] = sums[7] ^ input7;
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

    block blk_sum = zero_block;
    for (uint64_t i = 0; i < 8; i++) {
        blk_sum = blk_sum ^ sums[i];
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + Format(blk_sum));
#endif
    return blk_sum;
}

uint64_t DpfEvaluator::ComputeDotProductUint64Bitwise(const DpfKey &key, std::vector<uint64_t> &database) const {
    uint64_t nu     = params_.GetTerminateBitsize();
    uint64_t db_sum = 0;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint64_t i = 0; i < 3; ++i) {
        std::array<block, 2> expanded_seeds;
        std::array<bool, 2>  expanded_control_bits;
        next_seeds.resize(1U << (i + 1));
        next_control_bits.resize(1U << (i + 1));
        for (size_t j = 0; j < start_seeds.size(); j++) {
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[fss::kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[fss::kRight];
            next_control_bits[j * 2]     = expanded_control_bits[fss::kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[fss::kRight];
        }
        start_seeds        = std::move(next_seeds);
        start_control_bits = std::move(next_control_bits);
    }

    // Initialize the variables
    uint64_t current_level = 0;
    uint64_t current_idx   = 0;
    uint64_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint64_t last_idx      = 1U << last_depth;

    // Store the seeds and control bits
    std::array<block, 8>              expanded_seeds = {zero_block, zero_block, zero_block, zero_block,
                                                        zero_block, zero_block, zero_block, zero_block};
    std::array<bool, 8>               expanded_control_bits;
    std::vector<std::array<block, 8>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 8>>  prev_control_bits(last_depth + 1);

    // Evaluate the DPF key
    for (uint64_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint64_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint64_t i = 0; i < 8; ++i) {
                expanded_control_bits[i] = GetLsb(expanded_seeds[i]);
                SetLsbZero(expanded_seeds[i]);
            }

            // Apply correction word if control bit is true
            bool  cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            block cw_seed        = key.cw_seed[current_level + 3];
            expanded_seeds[0] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][0]]);
            expanded_seeds[1] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][1]]);
            expanded_seeds[2] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][2]]);
            expanded_seeds[3] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][3]]);
            expanded_seeds[4] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][4]]);
            expanded_seeds[5] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][5]]);
            expanded_seeds[6] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][6]]);
            expanded_seeds[7] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][7]]);

            for (uint64_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint64_t i = 0; i < 8; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        // Seed expansion for the final output
        G_.Expand(prev_seeds[current_level], prev_seeds[current_level], true);

        for (uint64_t j = 0; j < 8; ++j) {
            block    output_seed = prev_seeds[current_level][j] ^ (zero_and_all_one[prev_control_bits[current_level][j]] & key.output);
            uint64_t low         = output_seed.get<uint64_t>()[0];
            uint64_t high        = output_seed.get<uint64_t>()[1];
            uint64_t base_idx    = (j * last_idx + current_idx) * 128;

            for (int b = 0; b < 64; ++b) {
                uint64_t bit  = (low >> b) & 1ULL;
                uint64_t mask = 0ULL - bit;
                db_sum ^= (database[base_idx + b] & mask);
            }
            for (int b = 0; b < 64; ++b) {
                uint64_t bit  = (high >> b) & 1ULL;
                uint64_t mask = 0ULL - bit;
                db_sum ^= (database[base_idx + 64 + b] & mask);
            }
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + ToString(db_sum));
#endif
    return db_sum;
}

uint64_t DpfEvaluator::EvaluateFullDomainThenDotProduct(const DpfKey &key, std::vector<block> &outputs, std::vector<uint64_t> &database) const {
    // Evaluate the FDE
    EvaluateFullDomain(key, outputs);
    uint64_t db_sum = 0;

    // -------
    // Pattern 1
    // -------
    // // reinterpret_cast to raw bytes of 'outputs' buffer:
    // // each 'block' is assumed to be 16 bytes (128 bits).
    // const uint8_t *outputs_bytes = reinterpret_cast<const uint8_t *>(outputs.data());

    // // Let B = outputs.size(), so database.size() == B * 128.
    // const size_t B = outputs.size();
    // for (size_t block_index = 0; block_index < B; ++block_index) {
    //     // Pointer to the 16 bytes corresponding to this block:
    //     //   outputs_bytes + block_index * 16
    //     const uint8_t *block_ptr = outputs_bytes + (block_index * 16);

    //     // Now loop over the 128 bits within this block:
    //     for (size_t bit_index = 0; bit_index < 128; ++bit_index) {
    //         // Compute byte index and bit-in-byte from bit_index:
    //         size_t byte_index  = bit_index >> 3;     // bit_index / 8
    //         size_t bit_in_byte = bit_index & 0x7;    // bit_index % 8

    //         uint8_t  output_byte = block_ptr[byte_index];
    //         uint64_t bit         = (output_byte >> bit_in_byte) & 1U;

    //         // Corresponding index in 'database':
    //         size_t db_idx = block_index * 128 + bit_index;

    //         // If the bit is 1, XOR database[db_idx] into db_sum:
    //         // (database[db_idx] * bit) is faster than branchy "if (bit)".
    //         db_sum ^= (database[db_idx] * bit);
    //     }
    // }

    // -------
    // Pattern 2
    // -------
    // const size_t B = outputs.size();    // ブロック数
    // for (size_t block_index = 0; block_index < B; ++block_index) {
    //     // 1ブロック（16バイト=128ビット）分のデータをバッファにコピー
    //     alignas(16) uint8_t buffer[16];
    //     _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer),
    //                      outputs[block_index]);

    //     // 各ビットをチェックして database と内積（XOR）を取る
    //     // database 中の対応位置は (block_index * 128 + bit_index)
    //     for (size_t bit_index = 0; bit_index < 128; ++bit_index) {
    //         // ビット位置からバイトインデックス／バイト内ビット位置を求める
    //         //   byte_index   = bit_index / 8  <=> bit_index >> 3
    //         //   bit_in_byte  = bit_index % 8  <=> bit_index & 0x7
    //         const size_t byte_index  = bit_index >> 3;     // 8 ビット＝1 バイト
    //         const size_t bit_in_byte = bit_index & 0x7;    // 0～7 の範囲

    //         // バッファから該当バイトを読み出し、ビットを取り出す
    //         const uint8_t  output_byte = buffer[byte_index];
    //         const uint64_t bit         = (output_byte >> bit_in_byte) & 1U;

    //         // database上の対応インデックス
    //         const size_t db_idx = block_index * 128 + bit_index;

    //         // ビットが 1 のときだけ database の値を XOR に寄与させる
    //         //   (database[db_idx] * bit) を使うことで条件分岐なしに済む
    //         db_sum ^= (database[db_idx] * bit);
    //     }
    // }

    // -------
    // Pattern 3: This is the best performance
    // -------
    // Optimized bit extraction + mask-based accumulation
    size_t num_blocks = outputs.size();    // = num_shares / 128
    for (size_t i = 0; i < num_blocks; ++i) {
        // Load one 128-bit block into registers
        uint64_t low  = outputs[i].get<uint64_t>()[0];
        uint64_t high = outputs[i].get<uint64_t>()[1];

        // Process all 128 bits in this block
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask = 0ULL - ((low >> j) & 1ULL);
            db_sum ^= (database[i * 128 + j] & mask);
        }
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask = 0ULL - ((high >> j) & 1ULL);
            db_sum ^= (database[i * 128 + 64 + j] & mask);
        }
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + ToString(db_sum));
#endif
    return db_sum;
}

void DpfEvaluator::FullDomainRecursion(const DpfKey &key, std::vector<block> &outputs) const {
    uint64_t nu = params_.GetTerminateBitsize();

    // Get the seed and control bit from the given DPF key
    block seed        = key.init_seed;
    bool  control_bit = key.party_id != 0;

    Traverse(seed, control_bit, key, nu, 0, outputs);
}

void DpfEvaluator::FullDomainIterativeSingleBatch(const DpfKey &key, std::vector<block> &outputs) const {
    uint64_t nu            = params_.GetTerminateBitsize();
    uint64_t remaining_bit = params_.GetInputBitsize() - nu;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint64_t i = 0; i < 3; ++i) {
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
    uint64_t current_level = 0;
    uint64_t current_idx   = 0;
    uint64_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint64_t last_idx      = 1U << last_depth;

    // Store the seeds and control bits
    std::array<block, 8>              expanded_seeds;
    std::array<bool, 8>               expanded_control_bits;
    std::vector<std::array<block, 8>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 8>>  prev_control_bits(last_depth + 1);

    // Evaluate the DPF key
    for (uint64_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint64_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint64_t i = 0; i < 8; ++i) {
                expanded_control_bits[i] = GetLsb(expanded_seeds[i]);
                SetLsbZero(expanded_seeds[i]);
            }

#if LOG_LEVEL >= LOG_LEVEL_TRACE
            std::string level_str = "|Level=" + ToString(current_level) + "| ";
            for (uint64_t i = 0; i < 8; ++i) {
                Logger::TraceLog(LOC, level_str + "Current bit: " + ToString(current_bit));
                Logger::TraceLog(LOC, level_str + "Current seed (" + ToString(i) + "): " + Format(prev_seeds[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Current control bit (" + ToString(i) + "): " + ToString(prev_control_bits[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Expanded seed (" + ToString(i) + "): " + Format(expanded_seeds[i]));
                Logger::TraceLog(LOC, level_str + "Expanded control bit (" + ToString(i) + "): " + ToString(expanded_control_bits[i]));
            }
#endif

            // Apply correction word if control bit is true
            bool  cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            block cw_seed        = key.cw_seed[current_level + 3];
            expanded_seeds[0] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][0]]);
            expanded_seeds[1] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][1]]);
            expanded_seeds[2] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][2]]);
            expanded_seeds[3] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][3]]);
            expanded_seeds[4] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][4]]);
            expanded_seeds[5] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][5]]);
            expanded_seeds[6] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][6]]);
            expanded_seeds[7] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][7]]);

            for (uint64_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint64_t i = 0; i < 8; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        // Seed expansion for the final output
        G_.Expand(prev_seeds[current_level], prev_seeds[current_level], true);

        if (remaining_bit == 2) {
            if (key.party_id) {
                for (uint64_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_sub_epi32(zero_block, _mm_add_epi32(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output));
                }
            } else {
                for (uint64_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_add_epi32(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output);
                }
            }
        } else if (remaining_bit == 3) {
            if (key.party_id) {
                for (uint64_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_sub_epi16(zero_block, _mm_add_epi16(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output));
                }
            } else {
                for (uint64_t i = 0; i < 8; ++i) {
                    outputs[i * last_idx + current_idx] = _mm_add_epi16(prev_seeds[current_level][i], zero_and_all_one[prev_control_bits[current_level][i]] & key.output);
                }
            }
        } else if (remaining_bit == 7) {
            for (uint64_t i = 0; i < 8; ++i) {
                outputs[i * last_idx + current_idx] = prev_seeds[current_level][i] ^ (zero_and_all_one[prev_control_bits[current_level][i]] & key.output);
            }
        } else {
            Logger::FatalLog(LOC, "Invalid remaining bit: " + ToString(remaining_bit));
            std::exit(EXIT_FAILURE);
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

#if LOG_LEVEL >= LOG_LEVEL_TRACE
    for (uint64_t i = 0; i < (outputs.size() > 16 ? 16 : outputs.size()); ++i) {
        Logger::TraceLog(LOC, "Output seed (" + ToString(i) + "): " + Format(outputs[i]));
    }
#endif
}

void DpfEvaluator::FullDomainNaive(const DpfKey &key, std::vector<uint64_t> &outputs) const {
    // Evaluate the DPF key for all possible x values
    for (uint64_t x = 0; x < (1U << params_.GetInputBitsize()); x++) {
        outputs[x] = EvaluateAtNaive(key, x);
    }
}

void DpfEvaluator::Traverse(const block &current_seed, const bool current_control_bit, const DpfKey &key, uint64_t i, uint64_t j, std::vector<block> &outputs) const {
    uint64_t nu = params_.GetTerminateBitsize();

    if (i > 0) {
        // Evaluate the DPF key
        std::array<block, 2> expanded_seeds;           // expanded_seeds[keep or lose]
        std::array<bool, 2>  expanded_control_bits;    // expanded_control_bits[keep or lose]

        EvaluateNextSeed(nu - i, current_seed, current_control_bit, expanded_seeds, expanded_control_bits, key);

        // Traverse the left and right subtrees
        Traverse(expanded_seeds[kLeft], expanded_control_bits[kLeft], key, i - 1, j, outputs);
        Traverse(expanded_seeds[kRight], expanded_control_bits[kRight], key, i - 1, j + (1U << (i - 1)), outputs);
    } else {
        // Compute the output block
        outputs[j] = ComputeOutputBlock(current_seed, current_control_bit, key);
    }
}

// Compute the mask block and remaining bits
block DpfEvaluator::ComputeOutputBlock(const block &final_seed, bool final_control_bit, const DpfKey &key) const {
    // Compute the remaining bits
    block    mask          = zero_and_all_one[final_control_bit];
    uint64_t remaining_bit = params_.GetInputBitsize() - params_.GetTerminateBitsize();
    block    output        = zero_block;

    // Seed expansion for the final output
    block expanded_seed;
    G_.Expand(final_seed, expanded_seed, true);

    if (remaining_bit == 2) {
        // Reduce 2 levels (2^2=4 nodes) of the tree (Additive share)
        if (key.party_id) {
            output = _mm_sub_epi32(zero_block, _mm_add_epi32(expanded_seed, (mask & key.output)));
        } else {
            output = _mm_add_epi32(expanded_seed, (mask & key.output));
        }
    } else if (remaining_bit == 3) {
        // Reduce 3 levels (2^3=8 nodes) of the tree (Additive share)
        if (key.party_id) {
            output = _mm_sub_epi16(zero_block, _mm_add_epi16(expanded_seed, (mask & key.output)));
        } else {
            output = _mm_add_epi16(expanded_seed, (mask & key.output));
        }
    } else if (remaining_bit == 7) {
        // Reduce 7 levels (2^7=128 nodes) of the tree (Additive share)
        output = expanded_seed ^ (mask & key.output);
    } else {
        Logger::FatalLog(LOC, "Unsupported termination bitsize: " + ToString(remaining_bit));
        std::exit(EXIT_FAILURE);
    }

    return output;
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
