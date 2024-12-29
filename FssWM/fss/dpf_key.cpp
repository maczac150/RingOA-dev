#include "dpf_key.hpp"

#include <algorithm>

#include "utils/logger.hpp"
#include "utils/utils.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

std::string GetEvalTypeString(const EvalType eval_type) {
    switch (eval_type) {
        case EvalType::kNaive:
            return "Naive";
        case EvalType::kRec:
            return "Recursive";
        case EvalType::kNonRec:
            return "Non-Recursive";
        case EvalType::kNonRec_ParaEnc:
            return "Non-Recursive_ParaEnc";
        case EvalType::kNonRec_HalfPRG:
            return "Non-Recursive_HalfPRG";
        default:
            return "Unknown";
    }
}

DpfParameters::DpfParameters(const uint32_t n, const uint32_t e, const bool enable_et, EvalType eval_type)
    : input_bitsize_(n), element_bitsize_(e), enable_et_(enable_et) {
    AdjustParameters(eval_type);
    ComputeTerminateLevel();
    DecideFdeEvalType(eval_type);
    if (!ValidateParameters()) {
        utils::Logger::FatalLog(LOC, "Invalid DPF parameters");
        exit(EXIT_FAILURE);
    }
}

void DpfParameters::AdjustParameters(EvalType &eval_type) {
    // Restriction for domain size
    if (input_bitsize_ <= kSmallDomainSize && enable_et_) {
        utils::Logger::WarnLog(LOC, "Disabling early termination for small input bitsize (<=" + std::to_string(kSmallDomainSize) + "bit): ET OFF");
        enable_et_ = false;
    }

    // Restriction for evaluation type
    if (eval_type == EvalType::kNaive && enable_et_) {
        utils::Logger::WarnLog(LOC, "Disabling early termination for naive approach: ET OFF");
        enable_et_ = false;
    }

    if (!enable_et_ && eval_type != EvalType::kNaive) {
        utils::Logger::WarnLog(LOC, "Early termination is disabled: Switching to naive approach: EvalType: " + GetEvalTypeString(eval_type) + " -> Naive");
        eval_type = EvalType::kNaive;
    }
}

void DpfParameters::ComputeTerminateLevel() {
    if (enable_et_) {
        int32_t nu = 0;
        if (element_bitsize_ == 1) {
            nu = static_cast<int32_t>(input_bitsize_) - 7;    // 2^7 = 128
        } else {
            if (input_bitsize_ < 17) {
                nu = static_cast<int32_t>(input_bitsize_) - 3;    // Split seed (128 bits) 2^3 = 8 blocks
            } else if (input_bitsize_ < 33) {
                nu = static_cast<int32_t>(input_bitsize_) - 2;    // Split seed (128 bits) 2^2 = 4 blocks
            }
        }
        terminate_bitsize_ = std::max(nu, 0);
    } else {
        terminate_bitsize_ = input_bitsize_;
    }
}

void DpfParameters::DecideFdeEvalType(EvalType eval_type) {
    if (enable_et_) {
        if (element_bitsize_ == 1) {
            if (input_bitsize_ < 10) {    // 7 (early termination) + 3 (non-recursive)
                utils::Logger::WarnLog(LOC, "Switching to non-recursive approach for the domain size less than 10 bits: EvalType: " + GetEvalTypeString(eval_type) + " -> Non-Recursive");
                eval_type = EvalType::kNonRec;
            } else if (eval_type == EvalType::kNonRec_ParaEnc && input_bitsize_ < 11) {    // 7+1 (early termination) + 3 (non-recursive)
                utils::Logger::WarnLog(LOC, "Switching to non-recursive approach for the domain size less than 11 bits: EvalType: " + GetEvalTypeString(eval_type) + " -> Non-Recursive");
                eval_type = EvalType::kNonRec;
            }
        }
    } else {
        if (eval_type != EvalType::kNaive) {
            utils::Logger::WarnLog(LOC, "Early termination is disabled: Switching to naive approach: EvalType: " + GetEvalTypeString(eval_type) + " -> Naive");
            eval_type = EvalType::kNaive;
        }
    }
    fde_type_ = eval_type;
}

bool DpfParameters::ValidateParameters() const {
    bool valid = true;
    if (input_bitsize_ == 0 || element_bitsize_ == 0) {
        valid = false;
        utils::Logger::FatalLog(LOC, "The input bitsize and element bitsize must be greater than 0");
    }
    if (input_bitsize_ > 32) {
        valid = false;
        utils::Logger::FatalLog(LOC, "The input bitsize must be less than or equal to 32 (current: " + std::to_string(input_bitsize_) + ")");
    }
    if (enable_et_) {
        if (terminate_bitsize_ > input_bitsize_) {
            valid = false;
            utils::Logger::FatalLog(LOC,
                                    "terminate_bitsize_ (" + std::to_string(terminate_bitsize_) +
                                        ") must be <= input_bitsize_ (" + std::to_string(input_bitsize_) +
                                        ") when early termination is enabled.");
        }
    } else {
        if (terminate_bitsize_ != input_bitsize_) {
            valid = false;
            utils::Logger::FatalLog(LOC,
                                    "terminate_bitsize_ (" + std::to_string(terminate_bitsize_) +
                                        ") must be == input_bitsize_ (" + std::to_string(input_bitsize_) +
                                        ") when early termination is disabled.");
        }
    }
    if (fde_type_ == EvalType::kNaive && enable_et_) {
        valid = false;
        utils::Logger::FatalLog(LOC, "EvalType is Naive, but enable_et_ is true. This is invalid.");
    }
    return valid;
}

void DpfParameters::ReconfigureParameters(const uint32_t n, const uint32_t e, const bool enable_et, EvalType eval_type) {
    input_bitsize_   = n;
    element_bitsize_ = e;
    enable_et_       = enable_et;

    AdjustParameters(eval_type);
    ComputeTerminateLevel();
    DecideFdeEvalType(eval_type);

    if (!ValidateParameters()) {
        utils::Logger::FatalLog(LOC, "Invalid DPF parameters in Reconfigure()");
        exit(EXIT_FAILURE);
    }
}

void DpfParameters::PrintDpfParameters() const {
    std::ostringstream oss;
    oss << "[DPF Parameters] (Input, Output, Terminate): (" << input_bitsize_ << ", " << element_bitsize_ << ", " << terminate_bitsize_ << ") bit";
    oss << " (Early termination: " << (enable_et_ ? "ON" : "OFF") << ")";
    oss << " (EvalType: " << GetEvalTypeString(fde_type_) << ")";
    utils::Logger::InfoLog(LOC, oss.str());
}

DpfKey::DpfKey(const uint32_t id, const DpfParameters &params)
    : party_id(id), init_seed(zero_block), cw_length(params.GetTerminateBitsize()), cw_seed(new block[cw_length]), cw_control_left(new bool[cw_length]), cw_control_right(new bool[cw_length]), output(zero_block), params_(params) {
    for (uint32_t i = 0; i < cw_length; i++) {
        cw_seed[i]          = zero_block;
        cw_control_left[i]  = false;
        cw_control_right[i] = false;
    }
}

void DpfKey::PrintDpfKey(const bool debug) const {
#ifdef LOG_LEVEL_DEBUG
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("DPF Key"), debug);
    std::string et_status = params_.GetEnableEarlyTermination() ? "ON" : "OFF";
    utils::Logger::DebugLog(LOC, "Party ID: " + std::to_string(this->party_id), debug);
    utils::Logger::DebugLog(LOC, "Early termination: " + et_status, debug);
    utils::Logger::DebugLog(LOC, "Initial seed: " + ToString(init_seed), debug);
    utils::Logger::DebugLog(LOC, utils::Logger::StrWithSep("Correction words"), debug);
    for (uint32_t i = 0; i < this->cw_length; i++) {
        utils::Logger::DebugLog(LOC, "Level(" + std::to_string(i) + ") Seed: " + ToString(this->cw_seed[i]), debug);
        utils::Logger::DebugLog(LOC, "Level(" + std::to_string(i) + ") Control bit (L, R): " + std::to_string(this->cw_control_left[i]) + ", " + std::to_string(this->cw_control_right[i]), debug);
    }
    utils::Logger::DebugLog(LOC, "Output: " + ToString(output), debug);
    utils::Logger::DebugLog(LOC, utils::kDash, debug);
#endif
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
