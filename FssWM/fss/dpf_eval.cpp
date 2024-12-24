#include "dpf_eval.hpp"

#include "prg.hpp"
#include "utils/logger.hpp"
#include "utils/utils.hpp"

namespace {

// Create a pseudo-random generator
fsswm::fss::prg::PseudoRandomGenerator *G = new fsswm::fss::prg::PseudoRandomGenerator(fsswm::fss::prg::kPrgKeySeedLeft, fsswm::fss::prg::kPrgKeySeedRight);

}    // namespace

namespace fsswm {
namespace fss {
namespace dpf {

uint32_t DpfEvaluator::EvaluateAt(DpfKey &key, uint32_t x) const {
    if (!ValidateInput(x)) {
        utils::Logger::FatalLog(LOCATION, "Invalid input value: x=" + std::to_string(x));
        exit(EXIT_FAILURE);
    }
    if (params_.enable_early_termination) {
        return EvaluateAtOptimized(key, x);
    } else {
        return EvaluateAtNaive(key, x);
    }
}

uint32_t DpfEvaluator::EvaluateAtNaive(DpfKey &key, uint32_t x) const {
    uint32_t n = params_.input_bitsize;
    uint32_t e = params_.element_bitsize;

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, utils::Logger::StrWithSep("Evaluate input with DPF key"), debug_);
    utils::Logger::DebugLog(LOCATION, "Party ID: " + std::to_string(key.party_id), debug_);
    utils::Logger::DebugLog(LOCATION, "Input: " + std::to_string(x), debug_);
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
        bool current_bit = (x & (1 << (n - i - 1))) != 0;
        seed             = expanded_seeds[current_bit];
        control_bit      = expanded_control_bits[current_bit];

#ifdef LOG_LEVEL_DEBUG
        std::string level_str = "|Level=" + std::to_string(i) + "| ";
        utils::Logger::DebugLog(LOCATION, level_str + "Current bit: " + std::to_string(current_bit), debug_);
        utils::Logger::DebugLog(LOCATION, level_str + "Next seed: " + ToString(seed), debug_);
        utils::Logger::DebugLog(LOCATION, level_str + "Next control bit: " + std::to_string(control_bit), debug_);
#endif
    }
    // Compute the final output
    uint32_t output = utils::Pow(-1, key.party_id) * (Convert(seed, e) + (control_bit * Convert(key.output, e)));
    return utils::Mod(output, e);
}

uint32_t DpfEvaluator::EvaluateAtOptimized(DpfKey &key, uint32_t x) const {
    uint32_t n = params_.input_bitsize;
    uint32_t e = params_.element_bitsize;
}

bool DpfEvaluator::ValidateInput(const uint32_t x) const {
    bool valid = true;
    if (x >= (1U << params_.input_bitsize)) {
        valid = false;
    }
    return valid;
}

void DpfEvaluator::EvaluateNextSeed(
    uint32_t current_level, block &current_seed, bool &current_control_bit,
    std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
    DpfKey &key) const {
    // Expand the seed and control bits
    G->DoubleExpand(current_seed, expanded_seeds);
    expanded_control_bits[kLeft]  = getLSB(expanded_seeds[kLeft]);
    expanded_control_bits[kRight] = getLSB(expanded_seeds[kRight]);

#ifdef LOG_LEVEL_DEBUG
    std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
    utils::Logger::DebugLog(LOCATION, level_str + "Current seed: " + ToString(current_seed), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "Current control bit: " + std::to_string(current_control_bit), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "Expanded seed (L): " + ToString(expanded_seeds[kLeft]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "Expanded seed (R): " + ToString(expanded_seeds[kRight]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "Expanded control bit (L, R): " + std::to_string(expanded_control_bits[kLeft]) + ", " + std::to_string(expanded_control_bits[kRight]), debug_);
#endif

    // Apply correction word if control bit is true
    expanded_seeds[kLeft]         = expanded_seeds[kLeft] ^ (key.cw_seed[current_level] & zero_and_all_one[current_control_bit]);
    expanded_seeds[kRight]        = expanded_seeds[kRight] ^ (key.cw_seed[current_level] & zero_and_all_one[current_control_bit]);
    expanded_control_bits[kLeft]  = expanded_control_bits[kLeft] ^ (key.cw_control_left[current_level] & current_control_bit);
    expanded_control_bits[kRight] = expanded_control_bits[kRight] ^ (key.cw_control_right[current_level] & current_control_bit);
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
