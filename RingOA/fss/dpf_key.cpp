#include "dpf_key.h"

#include <cstring>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/to_string.h"

namespace ringoa {
namespace fss {
namespace dpf {

DpfParameters::DpfParameters(const uint64_t n, const uint64_t e, EvalType eval_type, OutputType output_mode)
    : input_bitsize_(n),
      element_bitsize_(e),
      enable_et_(true),
      fde_type_(eval_type),
      output_mode_(output_mode) {
    Resolve_();
    ValidateOrThrow_();
}

void DpfParameters::ReconfigureParameters(const uint64_t n, const uint64_t e, EvalType eval_type, OutputType output_mode) {
    input_bitsize_   = n;
    element_bitsize_ = e;
    enable_et_       = true;
    fde_type_        = eval_type;
    output_mode_     = output_mode;
    Resolve_();
    ValidateOrThrow_();
}

void DpfParameters::Resolve_() {
    // Small-n -> Naive
    if ((element_bitsize_ == 1 && input_bitsize_ < 10) ||
        (element_bitsize_ > 1 && input_bitsize_ <= 8)) {
        if (fde_type_ != EvalType::kNaive) {
            Logger::WarnLog(LOC, "Switching to naive evaluation: EvalType -> Naive");
        }
        fde_type_ = EvalType::kNaive;
    }

    // Disable ET for Naive / IterDepthFirst
    if (fde_type_ == EvalType::kNaive || fde_type_ == EvalType::kIterDepthFirst) {
        if (enable_et_) {
            Logger::WarnLog(LOC, "Disabling early termination for non-ET strategy: ET OFF");
        }
        enable_et_   = false;
        output_mode_ = OutputType::kShiftedAdditive;    // keep only if truly required
    }

    // Compute terminate_bitsize_
    if (enable_et_) {
        int32_t nu = 0;
        if (element_bitsize_ == 1) {
            nu = static_cast<int32_t>(input_bitsize_) - 7;    // 2^7
        } else if (input_bitsize_ < 17) {
            nu = static_cast<int32_t>(input_bitsize_) - 3;    // 8 blocks
            if (output_mode_ == OutputType::kSingleBitMask) {
                Logger::WarnLog(LOC, "Switching output to Additive for e!=1: OutputType -> ShiftedAdditive");
                output_mode_ = OutputType::kShiftedAdditive;
            }
        } else if (input_bitsize_ < 33) {
            nu = static_cast<int32_t>(input_bitsize_) - 2;    // 4 blocks
            if (output_mode_ == OutputType::kSingleBitMask) {
                Logger::WarnLog(LOC, "Switching output to Additive for e!=1: OutputType -> ShiftedAdditive");
                output_mode_ = OutputType::kShiftedAdditive;
            }
        }
        terminate_bitsize_ = static_cast<uint64_t>(std::max(nu, 0));
    } else {
        terminate_bitsize_ = input_bitsize_;
    }
}

void DpfParameters::ValidateOrThrow_() const {
    if (input_bitsize_ == 0 || element_bitsize_ == 0) {
        throw std::invalid_argument("input_bitsize and element_bitsize must be > 0");
    }
    if (input_bitsize_ > 32) {
        throw std::invalid_argument("input_bitsize must be <= 32 (got " + ToString(input_bitsize_) + ")");
    }
    if (enable_et_) {
        if (terminate_bitsize_ > input_bitsize_) {
            throw std::invalid_argument("nu (" + ToString(terminate_bitsize_) + ") must be <= n (" + ToString(input_bitsize_) + ") when ET is enabled");
        }
    } else {
        if (terminate_bitsize_ != input_bitsize_) {
            throw std::invalid_argument("nu (" + ToString(terminate_bitsize_) + ") must equal n (" + ToString(input_bitsize_) + ") when ET is disabled");
        }
    }
    if (fde_type_ == EvalType::kNaive && enable_et_) {
        throw std::invalid_argument("EvalType::kNaive requires ET to be disabled");
    }
    if (element_bitsize_ != 1 && output_mode_ == OutputType::kSingleBitMask) {
        throw std::invalid_argument("OutputType::kSingleBitMask requires element_bitsize == 1");
    }
}

std::string DpfParameters::GetParametersInfo() const {
    std::ostringstream oss;
    oss << "(Input, Output, Terminate): (" << input_bitsize_ << ", " << element_bitsize_ << ", " << terminate_bitsize_ << ") bit";
    oss << " (Early termination: " << (enable_et_ ? "ON" : "OFF") << ")";
    oss << " (EvalType: " << GetEvalTypeString(fde_type_) << ")";
    oss << " (OutputType: " << GetOutputTypeString(output_mode_) << ")";
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

bool DpfKey::operator==(const DpfKey &rhs) const {
    if (party_id != rhs.party_id)
        return false;
    if (init_seed != rhs.init_seed)
        return false;
    if (cw_length != rhs.cw_length)
        return false;
    if (output != rhs.output)
        return false;

    for (uint64_t i = 0; i < cw_length; ++i) {
        if (cw_seed[i] != rhs.cw_seed[i])
            return false;
        if (cw_control_left[i] != rhs.cw_control_left[i])
            return false;
        if (cw_control_right[i] != rhs.cw_control_right[i])
            return false;
    }
    return true;
}

size_t DpfKey::CalculateSerializedSize() const {
    // Fix bool serialization to 1 byte per element for portability.
    size_t size = 0;
    size += sizeof(party_id);
    size += sizeof(init_seed);
    size += sizeof(cw_length);
    size += sizeof(block) * cw_length;
    size += cw_length;    // left controls (bytes)
    size += cw_length;    // right controls (bytes)
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
}    // namespace ringoa
