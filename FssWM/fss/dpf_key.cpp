#include "dpf_key.h"

#include <cstring>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/to_string.h"

namespace fsswm {
namespace fss {
namespace dpf {

DpfParameters::DpfParameters(const uint64_t n, const uint64_t e, EvalType eval_type, OutputMode output_mode)
    : input_bitsize_(n), element_bitsize_(e), enable_et_(true), fde_type_(eval_type), output_mode_(output_mode) {

    if ((element_bitsize_ == 1 && input_bitsize_ < 10) ||
        (element_bitsize_ > 1 && input_bitsize_ <= 8)) {
        if (fde_type_ != EvalType::kNaive) {
            Logger::WarnLog(LOC, "Switching to naive evaluation: EvalType -> Naive");
        }
        fde_type_ = EvalType::kNaive;
    }

    if (fde_type_ == EvalType::kNaive) {
        enable_et_   = false;
        output_mode_ = OutputMode::kAdditive;
        if (enable_et_) {
            Logger::WarnLog(LOC, "Disabling early termination for naive evaluation: ET OFF");
        }
    }

    if (enable_et_) {
        int32_t nu = 0;
        if (element_bitsize_ == 1) {
            nu = static_cast<int32_t>(input_bitsize_) - 7;    // 2^7 = 128
        } else if (input_bitsize_ < 17) {
            nu = static_cast<int32_t>(input_bitsize_) - 3;    // Split seed (128 bits) 2^3 = 8 blocks
            if (output_mode_ == OutputMode::kBinaryPoint) {
                output_mode_ = OutputMode::kAdditive;
                Logger::WarnLog(LOC, "Switching output mode to Additive for input bitsize != 1: OutputMode -> Additive");
            }
        } else if (input_bitsize_ < 33) {
            nu = static_cast<int32_t>(input_bitsize_) - 2;    // Split seed (128 bits) 2^2 = 4 blocks
            if (output_mode_ == OutputMode::kBinaryPoint) {
                output_mode_ = OutputMode::kAdditive;
                Logger::WarnLog(LOC, "Switching output mode to Additive for input bitsize != 1: OutputMode -> Additive");
            }
        }
        terminate_bitsize_ = std::max(nu, 0);
    } else {
        terminate_bitsize_ = input_bitsize_;
    }

    if (!ValidateParameters()) {
        Logger::FatalLog(LOC, "Invalid DPF parameters");
        std::exit(EXIT_FAILURE);
    }
}

bool DpfParameters::ValidateParameters() const {
    bool valid = true;
    if (input_bitsize_ == 0 || element_bitsize_ == 0) {
        valid = false;
        Logger::FatalLog(LOC, "The input bitsize and element bitsize must be greater than 0");
    }
    if (input_bitsize_ > 32) {
        valid = false;
        Logger::FatalLog(LOC, "The input bitsize must be less than or equal to 32 (current: " + ToString(input_bitsize_) + ")");
    }
    if (enable_et_) {
        if (terminate_bitsize_ > input_bitsize_) {
            valid = false;
            Logger::FatalLog(LOC,
                             "terminate_bitsize_ (" + ToString(terminate_bitsize_) +
                                 ") must be <= input_bitsize_ (" + ToString(input_bitsize_) +
                                 ") when early termination is enabled.");
        }
    } else {
        if (terminate_bitsize_ != input_bitsize_) {
            valid = false;
            Logger::FatalLog(LOC,
                             "terminate_bitsize_ (" + ToString(terminate_bitsize_) +
                                 ") must be == input_bitsize_ (" + ToString(input_bitsize_) +
                                 ") when early termination is disabled.");
        }
    }
    if (fde_type_ == EvalType::kNaive && enable_et_) {
        valid = false;
        Logger::FatalLog(LOC, "EvalType is Naive, but enable_et_ is true. This is invalid.");
    }
    return valid;
}

void DpfParameters::ReconfigureParameters(const uint64_t n, const uint64_t e, EvalType eval_type, OutputMode output_mode) {
    input_bitsize_   = n;
    element_bitsize_ = e;
    enable_et_       = true;

    if ((element_bitsize_ == 1 && input_bitsize_ < 10) ||
        (element_bitsize_ > 1 && input_bitsize_ <= 8)) {
        if (fde_type_ != EvalType::kNaive) {
            Logger::WarnLog(LOC, "Switching to naive evaluation: EvalType -> Naive");
        }
        fde_type_ = EvalType::kNaive;
    }

    if (fde_type_ == EvalType::kNaive) {
        enable_et_   = false;
        output_mode_ = OutputMode::kAdditive;
        if (enable_et_) {
            Logger::WarnLog(LOC, "Disabling early termination for naive evaluation: ET OFF");
        }
    }

    if (enable_et_) {
        int32_t nu = 0;
        if (element_bitsize_ == 1) {
            nu = static_cast<int32_t>(input_bitsize_) - 7;    // 2^7 = 128
        } else if (input_bitsize_ < 17) {
            nu = static_cast<int32_t>(input_bitsize_) - 3;    // Split seed (128 bits) 2^3 = 8 blocks
            if (output_mode_ == OutputMode::kBinaryPoint) {
                output_mode_ = OutputMode::kAdditive;
                Logger::WarnLog(LOC, "Switching output mode to Additive for input bitsize != 1: OutputMode -> Additive");
            }
        } else if (input_bitsize_ < 33) {
            nu = static_cast<int32_t>(input_bitsize_) - 2;    // Split seed (128 bits) 2^2 = 4 blocks
            if (output_mode_ == OutputMode::kBinaryPoint) {
                output_mode_ = OutputMode::kAdditive;
                Logger::WarnLog(LOC, "Switching output mode to Additive for input bitsize != 1: OutputMode -> Additive");
            }
        }
        terminate_bitsize_ = std::max(nu, 0);
    } else {
        terminate_bitsize_ = input_bitsize_;
    }

    if (!ValidateParameters()) {
        Logger::FatalLog(LOC, "Invalid DPF parameters");
        std::exit(EXIT_FAILURE);
    }
}

std::string DpfParameters::GetParametersInfo() const {
    std::ostringstream oss;
    oss << "(Input, Output, Terminate): (" << input_bitsize_ << ", " << element_bitsize_ << ", " << terminate_bitsize_ << ") bit";
    oss << " (Early termination: " << (enable_et_ ? "ON" : "OFF") << ")";
    oss << " (EvalType: " << GetEvalTypeString(fde_type_) << ")";
    oss << " (OutputMode: " << GetOutputModeString(output_mode_) << ")";
    return oss.str();
}

void DpfParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[DPF Parameters] " + GetParametersInfo());
}

DpfKey::DpfKey(const uint64_t id, const DpfParameters &params)
    : party_id(id),
      init_seed(zero_block),
      cw_length(params.GetTerminateBitsize()),
      cw_seed(std::make_unique<block[]>(cw_length)),
      cw_control_left(std::make_unique<bool[]>(cw_length)),
      cw_control_right(std::make_unique<bool[]>(cw_length)),
      output(zero_block),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
    std::fill(cw_seed.get(), cw_seed.get() + cw_length, zero_block);
    std::fill(cw_control_left.get(), cw_control_left.get() + cw_length, false);
    std::fill(cw_control_right.get(), cw_control_right.get() + cw_length, false);
}

size_t DpfKey::CalculateSerializedSize() const {
    size_t size = 0;

    size += sizeof(party_id);
    size += sizeof(init_seed);
    size += sizeof(cw_length);
    size += sizeof(block) * cw_length;
    size += sizeof(bool) * cw_length * 2;
    size += sizeof(output);

    return size;
}

void DpfKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing DPF key");
#endif

    // Party ID and Initial seed
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&party_id), reinterpret_cast<const uint8_t *>(&party_id) + sizeof(party_id));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&init_seed), reinterpret_cast<const uint8_t *>(&init_seed) + sizeof(init_seed));

    // Correction words
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&cw_length), reinterpret_cast<const uint8_t *>(&cw_length) + sizeof(cw_length));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_seed.get()), reinterpret_cast<const uint8_t *>(cw_seed.get()) + sizeof(block) * cw_length);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_control_left.get()), reinterpret_cast<const uint8_t *>(cw_control_left.get()) + sizeof(bool) * cw_length);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_control_right.get()), reinterpret_cast<const uint8_t *>(cw_control_right.get()) + sizeof(bool) * cw_length);

    // Output
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&output), reinterpret_cast<const uint8_t *>(&output) + sizeof(output));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void DpfKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing DPF key");
#endif
    size_t offset = 0;

    // Party ID and Initial seed
    std::memcpy(&party_id, buffer.data() + offset, sizeof(party_id));
    offset += sizeof(party_id);
    std::memcpy(&init_seed, buffer.data() + offset, sizeof(init_seed));
    offset += sizeof(init_seed);
    std::memcpy(&cw_length, buffer.data() + offset, sizeof(cw_length));
    offset += sizeof(cw_length);

    // Correction Words
    cw_seed          = std::make_unique<block[]>(cw_length);
    cw_control_left  = std::make_unique<bool[]>(cw_length);
    cw_control_right = std::make_unique<bool[]>(cw_length);
    std::memcpy(cw_seed.get(), buffer.data() + offset, sizeof(block) * cw_length);
    offset += sizeof(block) * cw_length;
    std::memcpy(cw_control_left.get(), buffer.data() + offset, sizeof(bool) * cw_length);
    offset += sizeof(bool) * cw_length;
    std::memcpy(cw_control_right.get(), buffer.data() + offset, sizeof(bool) * cw_length);
    offset += sizeof(bool) * cw_length;

    // Output
    std::memcpy(&output, buffer.data() + offset, sizeof(output));
}

void DpfKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("DPF Key"));
        std::string et_status = params_.GetEnableEarlyTermination() ? "ON" : "OFF";
        Logger::DebugLog(LOC, "Party ID: " + ToString(this->party_id));
        Logger::DebugLog(LOC, "Early termination: " + et_status);
        Logger::DebugLog(LOC, "Initial seed: " + Format(init_seed));
        Logger::DebugLog(LOC, Logger::StrWithSep("Correction words"));
        for (uint64_t i = 0; i < this->cw_length; ++i) {
            Logger::DebugLog(LOC, "Level(" + ToString(i) + ") Seed: " + Format(this->cw_seed[i]));
            Logger::DebugLog(LOC, "Level(" + ToString(i) + ") Control bit (L, R): " + ToString(this->cw_control_left[i]) + ", " + ToString(this->cw_control_right[i]));
        }
        Logger::DebugLog(LOC, "Output: " + Format(output));
        Logger::DebugLog(LOC, kDash);
    } else {
        std::ostringstream oss;
        oss << "[DPF Key] P" << party_id << " (ET: " << (params_.GetEnableEarlyTermination() ? "ON" : "OFF") << ")";
        oss << " (CW: " << cw_length << ") (Init: " << Format(init_seed) << ")";
        Logger::DebugLog(LOC, oss.str());
    }
#endif
}

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm
