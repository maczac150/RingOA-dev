#include "dpf_key.hpp"

#include <algorithm>

#include "utils/logger.hpp"
#include "utils/utils.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

constexpr uint32_t kSecurityParameter = 128;

DpfParameters::DpfParameters(const uint32_t n, const uint32_t e, const bool enable_et, const bool debug)
    : input_bitsize(n), element_bitsize(e), enable_early_termination(enable_et), terminate_bitsize(ComputeTerminateLevel(enable_et)), debug(debug) {
    if (!ValidateParameters()) {
        utils::Logger::FatalLog(LOCATION, "Invalid DPF parameters");
        exit(EXIT_FAILURE);
    }
    if (enable_early_termination) {
        utils::Logger::DebugLog(LOCATION, "Early termination enabled", debug);
    } else {
        utils::Logger::DebugLog(LOCATION, "Early termination disabled", debug);
    }
}

uint32_t DpfParameters::ComputeTerminateLevel(const bool enable_et) const {
    if (enable_et) {
        int32_t nu = static_cast<int32_t>(std::min(std::ceil(input_bitsize - std::log2(kSecurityParameter / element_bitsize)), static_cast<double>(input_bitsize)));
        utils::Logger::DebugLog(LOCATION, "Terminate level: " + std::to_string(nu), debug);
        return (nu < 0) ? 0 : static_cast<uint32_t>(nu);
    } else {
        return input_bitsize;
    }
}

bool DpfParameters::ValidateParameters() const {
    bool valid = true;
    if (input_bitsize == 0 || element_bitsize == 0) {
        valid = false;
        utils::Logger::FatalLog(LOCATION, "The input bitsize and element bitsize must be greater than 0");
    }
    if (input_bitsize > 32) {
        valid = false;
        utils::Logger::FatalLog(LOCATION, "The input bitsize must be less than or equal to 32 (current: " + std::to_string(input_bitsize) + ")");
    }
    return valid;
}

DpfKey::DpfKey(const uint32_t id, const DpfParameters &params)
    : party_id(id), init_seed(zero_block), cw_length(params.terminate_bitsize), cw_seed(new block[cw_length]), cw_control_left(new bool[cw_length]), cw_control_right(new bool[cw_length]), output(zero_block), params_(params) {
    for (uint32_t i = 0; i < cw_length; i++) {
        cw_seed[i]          = zero_block;
        cw_control_left[i]  = false;
        cw_control_right[i] = false;
    }
}

void DpfKey::PrintDpfKey(const bool debug) const {
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOCATION, utils::Logger::StrWithSep("DPF Key"), debug);
    std::string et_status = params_.enable_early_termination ? "ON" : "OFF";
    utils::Logger::DebugLog(LOCATION, "Party ID: " + std::to_string(this->party_id), debug);
    utils::Logger::DebugLog(LOCATION, "Early termination: " + et_status, debug);
    utils::Logger::DebugLog(LOCATION, "Initial seed: " + ToString(init_seed), debug);
    utils::Logger::DebugLog(LOCATION, utils::Logger::StrWithSep("Correction words"), debug);
    for (uint32_t i = 0; i < this->cw_length; i++) {
        utils::Logger::DebugLog(LOCATION, "Level(" + std::to_string(i) + ") Seed: " + ToString(this->cw_seed[i]), debug);
        utils::Logger::DebugLog(LOCATION, "Level(" + std::to_string(i) + ") Control bit (L, R): " + std::to_string(this->cw_control_left[i]) + ", " + std::to_string(this->cw_control_right[i]), debug);
    }
    utils::Logger::DebugLog(LOCATION, "Output: " + ToString(output), debug);
    utils::Logger::DebugLog(LOCATION, utils::kDash, debug);
#endif
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
