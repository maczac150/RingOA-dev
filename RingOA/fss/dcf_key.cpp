#include "dcf_key.h"

#include <cstring>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/to_string.h"

namespace ringoa {
namespace fss {
namespace dcf {

DcfParameters::DcfParameters(const uint64_t n, const uint64_t e)
    : input_bitsize_(n), element_bitsize_(e) {

    if (!ValidateParameters()) {
        Logger::FatalLog(LOC, "Invalid DCF parameters");
        std::exit(EXIT_FAILURE);
    }
}

bool DcfParameters::ValidateParameters() const {
    bool valid = true;
    if (input_bitsize_ == 0 || element_bitsize_ == 0) {
        valid = false;
        Logger::FatalLog(LOC, "The input bitsize and element bitsize must be greater than 0");
    }
    if (input_bitsize_ > 32) {
        valid = false;
        Logger::FatalLog(LOC, "The input bitsize must be less than or equal to 32 (current: " + ToString(input_bitsize_) + ")");
    }
    return valid;
}

void DcfParameters::ReconfigureParameters(const uint64_t n, const uint64_t e) {
    input_bitsize_   = n;
    element_bitsize_ = e;

    if (!ValidateParameters()) {
        Logger::FatalLog(LOC, "Invalid DCF parameters");
        std::exit(EXIT_FAILURE);
    }
}

std::string DcfParameters::GetParametersInfo() const {
    std::ostringstream oss;
    oss << "(Input, Output): (" << input_bitsize_ << ", " << element_bitsize_ << ")";
    return oss.str();
}

void DcfParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[DCF Parameters] " + GetParametersInfo());
}

DcfKey::DcfKey(const uint64_t id, const DcfParameters &params)
    : party_id(id),
      init_seed(zero_block),
      cw_length(params.GetInputBitsize()),
      cw_seed(std::make_unique<block[]>(cw_length)),
      cw_control_left(std::make_unique<bool[]>(cw_length)),
      cw_control_right(std::make_unique<bool[]>(cw_length)),
      cw_value(std::make_unique<uint64_t[]>(cw_length)),
      output(0),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
    std::fill(cw_seed.get(), cw_seed.get() + cw_length, zero_block);
    std::fill(cw_control_left.get(), cw_control_left.get() + cw_length, false);
    std::fill(cw_control_right.get(), cw_control_right.get() + cw_length, false);
    std::fill(cw_value.get(), cw_value.get() + cw_length, 0);
}

bool DcfKey::operator==(const DcfKey &rhs) const {
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
        if (cw_value[i] != rhs.cw_value[i])
            return false;
    }
    return true;
}

size_t DcfKey::CalculateSerializedSize() const {
    size_t size = 0;

    size += sizeof(party_id);
    size += sizeof(init_seed);
    size += sizeof(cw_length);
    size += sizeof(block) * cw_length;
    size += sizeof(bool) * cw_length * 2;
    size += sizeof(uint64_t) * cw_length;
    size += sizeof(output);

    return size;
}

void DcfKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing DCF key");
#endif

    // Party ID and Initial seed
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&party_id), reinterpret_cast<const uint8_t *>(&party_id) + sizeof(party_id));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&init_seed), reinterpret_cast<const uint8_t *>(&init_seed) + sizeof(init_seed));

    // Correction words
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&cw_length), reinterpret_cast<const uint8_t *>(&cw_length) + sizeof(cw_length));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_seed.get()), reinterpret_cast<const uint8_t *>(cw_seed.get()) + sizeof(block) * cw_length);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_control_left.get()), reinterpret_cast<const uint8_t *>(cw_control_left.get()) + sizeof(bool) * cw_length);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_control_right.get()), reinterpret_cast<const uint8_t *>(cw_control_right.get()) + sizeof(bool) * cw_length);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(cw_value.get()), reinterpret_cast<const uint8_t *>(cw_value.get()) + sizeof(uint64_t) * cw_length);

    // Output
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&output), reinterpret_cast<const uint8_t *>(&output) + sizeof(output));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void DcfKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing DCF key");
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
    cw_value         = std::make_unique<uint64_t[]>(cw_length);
    std::memcpy(cw_seed.get(), buffer.data() + offset, sizeof(block) * cw_length);
    offset += sizeof(block) * cw_length;
    std::memcpy(cw_control_left.get(), buffer.data() + offset, sizeof(bool) * cw_length);
    offset += sizeof(bool) * cw_length;
    std::memcpy(cw_control_right.get(), buffer.data() + offset, sizeof(bool) * cw_length);
    offset += sizeof(bool) * cw_length;
    std::memcpy(cw_value.get(), buffer.data() + offset, sizeof(uint64_t) * cw_length);
    offset += sizeof(uint64_t) * cw_length;

    // Output
    std::memcpy(&output, buffer.data() + offset, sizeof(output));
}

void DcfKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("DCF Key"));
        Logger::DebugLog(LOC, "Party ID: " + ToString(this->party_id));
        Logger::DebugLog(LOC, "Initial seed: " + Format(init_seed));
        Logger::DebugLog(LOC, Logger::StrWithSep("Correction words"));
        for (uint64_t i = 0; i < this->cw_length; ++i) {
            Logger::DebugLog(LOC, "Level(" + ToString(i) + ") Seed: " + Format(this->cw_seed[i]));
            Logger::DebugLog(LOC, "Level(" + ToString(i) + ") Control bit (L, R): " + ToString(this->cw_control_left[i]) + ", " + ToString(this->cw_control_right[i]));
            Logger::DebugLog(LOC, "Level(" + ToString(i) + ") Value: " + ToString(this->cw_value[i]));
        }
        Logger::DebugLog(LOC, "Output: " + ToString(output));
        Logger::DebugLog(LOC, kDash);
    } else {
        std::ostringstream oss;
        oss << "[DCF Key] P" << party_id << " (CW: " << cw_length << ") (Init: " << Format(init_seed) << ")";
        Logger::DebugLog(LOC, oss.str());
    }
#endif
}

}    // namespace dcf
}    // namespace fss
}    // namespace ringoa
