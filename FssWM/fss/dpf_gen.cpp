#include "dpf_gen.hpp"

#include "fss.hpp"
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

std::pair<DpfKey, DpfKey> DpfKeyGenerator::GenerateKeys(uint32_t alpha, uint32_t beta) const {
    // Validate the input values
    if (!ValidateInput(alpha, beta)) {
        utils::Logger::FatalLog(LOCATION, "Invalid input values: alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta));
        exit(EXIT_FAILURE);
    }

    // Initialize the DPF keys
    DpfKey                    key_0(0, params_);
    DpfKey                    key_1(1, params_);
    std::pair<DpfKey, DpfKey> key_pair = std::make_pair(std::move(key_0), std::move(key_1));

    // Generate the DPF key
    if (params_.enable_early_termination) {
        GenerateKeysOptimized(alpha, beta, key_pair);
    } else {
        GenerateKeysNaive(alpha, beta, key_pair);
    }

    return key_pair;
}

void DpfKeyGenerator::GenerateKeysNaive(uint32_t alpha, uint32_t beta, std::pair<DpfKey, DpfKey> &key_pair) const {
    uint32_t n = params_.input_bitsize;
    uint32_t e = params_.element_bitsize;

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, "Generating DPF key (naive approach)", debug_);
    utils::Logger::DebugLog(LOCATION, "Input bitsize: " + std::to_string(n), debug_);
    utils::Logger::DebugLog(LOCATION, "Element bitsize: " + std::to_string(e), debug_);
    utils::Logger::DebugLog(LOCATION, "Alpha: " + std::to_string(alpha), debug_);
    utils::Logger::DebugLog(LOCATION, "Beta: " + std::to_string(beta), debug_);
#endif

    // Set the initial seed and control bits
    block seed_0              = SetRandomBlock();
    block seed_1              = SetRandomBlock();
    bool  control_bit_0       = 0;
    bool  control_bit_1       = 1;
    key_pair.first.init_seed  = seed_0;
    key_pair.second.init_seed = seed_1;

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, "[P0] Initial seed: " + ToString(seed_0), debug_);
    utils::Logger::DebugLog(LOCATION, "[P0] Control bit: " + std::to_string(control_bit_0), debug_);
    utils::Logger::DebugLog(LOCATION, "[P1] Initial seed: " + ToString(seed_1), debug_);
    utils::Logger::DebugLog(LOCATION, "[P1] Control bit: " + std::to_string(control_bit_1), debug_);
#endif

    // Generate next seed and compute correction words
    for (uint32_t i = 0; i < n; i++) {
        bool current_bit = (alpha & (1 << (n - i - 1))) != 0;
        GenerateNextSeed(i, current_bit, seed_0, control_bit_0, seed_1, control_bit_1, key_pair);
    }

    // Set the output
    uint32_t result        = utils::Mod(utils::Pow(-1, control_bit_1) * (beta - Convert(seed_0, e) + Convert(seed_1, e)), e);
    block    output        = makeBlock(0, result);
    key_pair.first.output  = output;
    key_pair.second.output = output;

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, "Output: " + ToString(output), debug_);
    utils::AddNewLine(debug_);
    key_pair.first.PrintDpfKey(debug_);
    key_pair.second.PrintDpfKey(debug_);
#endif
}

void DpfKeyGenerator::GenerateKeysOptimized(uint32_t alpha, uint32_t beta, std::pair<DpfKey, DpfKey> &key_pair) const {
}

bool DpfKeyGenerator::ValidateInput(const uint32_t alpha, const uint32_t beta) const {
    bool valid = true;
    if (alpha >= (1U << params_.input_bitsize) || beta >= (1U << params_.element_bitsize)) {
        valid = false;
    }
    return valid;
}

void DpfKeyGenerator::GenerateNextSeed(const uint32_t current_level, const bool current_bit,
                                       block &current_seed_0, bool &current_control_bit_0,
                                       block &current_seed_1, bool &current_control_bit_1,
                                       std::pair<DpfKey, DpfKey> &key_pair) const {
#ifdef LOG_LEVEL_DEBUG
    std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
#endif

    std::array<block, 2> expanded_seed_0;           // expanded_seed_0[keep or lose]
    std::array<block, 2> expanded_seed_1;           // expanded_seed_1[keep or lose]
    std::array<bool, 2>  expanded_control_bit_0;    // expanded_control_bit_0[keep or lose]
    std::array<bool, 2>  expanded_control_bit_1;    // expanded_control_bit_1[keep or lose]
    block                seed_correction;           // seed_correction
    std::array<bool, 2>  control_bit_correction;    // control_bit_correction[keep or lose]

    // Expand the seed and control bits
    G->DoubleExpand(current_seed_0, expanded_seed_0);
    G->DoubleExpand(current_seed_1, expanded_seed_1);
    expanded_control_bit_0[kLeft]  = getLSB(expanded_seed_0[kLeft]);
    expanded_control_bit_0[kRight] = getLSB(expanded_seed_0[kRight]);
    expanded_control_bit_1[kLeft]  = getLSB(expanded_seed_1[kLeft]);
    expanded_control_bit_1[kRight] = getLSB(expanded_seed_1[kRight]);

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, level_str + "[P0] Expanded seed (L): " + ToString(expanded_seed_0[kLeft]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P0] Expanded seed (R): " + ToString(expanded_seed_0[kRight]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P0] Expanded control bit (L, R): " + std::to_string(expanded_control_bit_0[kLeft]) + ", " + std::to_string(expanded_control_bit_0[kRight]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P1] Expanded seed (L): " + ToString(expanded_seed_1[kLeft]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P1] Expanded seed (R): " + ToString(expanded_seed_1[kRight]), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P1] Expanded control bit (L, R): " + std::to_string(expanded_control_bit_1[kLeft]) + ", " + std::to_string(expanded_control_bit_1[kRight]), debug_);
#endif

    // Choose keep or lose path
    bool keep = current_bit, lose = !current_bit;
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, level_str + "Current bit: " + std::to_string(current_bit) + " (Keep: " + std::to_string(keep) + ", Lose: " + std::to_string(lose) + ")", debug_);
#endif

    // Compute seed correction
    seed_correction = expanded_seed_0[lose] ^ expanded_seed_1[lose];
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, level_str + "Seed correction: " + ToString(seed_correction), debug_);
#endif

    // Compute control bit correction
    control_bit_correction[kLeft]  = expanded_control_bit_0[kLeft] ^ expanded_control_bit_1[kLeft] ^ current_bit ^ 1;
    control_bit_correction[kRight] = expanded_control_bit_0[kRight] ^ expanded_control_bit_1[kRight] ^ current_bit;
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, level_str + "Correction control bit (L, R): " + std::to_string(control_bit_correction[kLeft]) + ", " + std::to_string(control_bit_correction[kRight]), debug_);
#endif

    // Set the correction word
    key_pair.first.cw_seed[current_level]           = seed_correction;
    key_pair.first.cw_control_left[current_level]   = control_bit_correction[kLeft];
    key_pair.first.cw_control_right[current_level]  = control_bit_correction[kRight];
    key_pair.second.cw_seed[current_level]          = seed_correction;
    key_pair.second.cw_control_left[current_level]  = control_bit_correction[kLeft];
    key_pair.second.cw_control_right[current_level] = control_bit_correction[kRight];

    // Update seed and control bits
    current_seed_0        = expanded_seed_0[keep];
    current_seed_1        = expanded_seed_1[keep];
    current_seed_0        = current_seed_0 ^ (seed_correction & zero_and_all_one[current_control_bit_0]);
    current_seed_1        = current_seed_1 ^ (seed_correction & zero_and_all_one[current_control_bit_1]);
    current_control_bit_0 = expanded_control_bit_0[keep] ^ (current_control_bit_0 & control_bit_correction[keep]);
    current_control_bit_1 = expanded_control_bit_1[keep] ^ (current_control_bit_1 & control_bit_correction[keep]);

#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, level_str + "[P0] Next seed: " + ToString(current_seed_0), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P0] Next control bit: " + std::to_string(current_control_bit_0), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P1] Next seed: " + ToString(current_seed_1), debug_);
    utils::Logger::DebugLog(LOCATION, level_str + "[P1] Next control bit: " + std::to_string(current_control_bit_1), debug_);
#endif
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
