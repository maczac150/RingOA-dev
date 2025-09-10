#include "ddcf.h"

#include <cstring>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace ringoa {
namespace proto {

void DdcfParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[DDCF Parameters] " + GetParametersInfo());
}

DdcfKey::DdcfKey(const uint64_t id, const DdcfParameters &params)
    : dcf_key(id, params.GetParameters()),
      mask(0), params_(params), serialized_size_(CalculateSerializedSize()) {
}

size_t DdcfKey::CalculateSerializedSize() const {
    size_t size = 0;

    size += dcf_key.GetSerializedSize();
    size += sizeof(mask);
    return size;
}

void DdcfKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing DDCF key");
#endif

    // Serialize the DDCF key
    std::vector<uint8_t> key_buffer;
    dcf_key.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the mask
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&mask), reinterpret_cast<const uint8_t *>(&mask) + sizeof(mask));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void DdcfKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing DDCF key");
#endif
    size_t offset = 0;

    // Deserialize the DDCF key
    size_t key_size = dcf_key.GetSerializedSize();
    dcf_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the mask
    std::memcpy(&mask, buffer.data() + offset, sizeof(mask));
}

void DdcfKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        if (detailed) {
            Logger::DebugLog(LOC, Logger::StrWithSep("DDCF Key [Party " + ToString(dcf_key.party_id) + "]"));
            dcf_key.PrintKey(true);
            Logger::DebugLog(LOC, "mask: " + ToString(mask));
            Logger::DebugLog(LOC, kDash);
        } else {
            Logger::DebugLog(LOC, "DDCF Key [Party " + ToString(dcf_key.party_id) + "]");
            dcf_key.PrintKey(false);
            Logger::DebugLog(LOC, "mask: " + ToString(mask));
        }
    }
#endif
}

DdcfKeyGenerator::DdcfKeyGenerator(const DdcfParameters &params)
    : params_(params), gen_(params.GetParameters()) {
}

std::pair<DdcfKey, DdcfKey> DdcfKeyGenerator::GenerateKeys(uint64_t alpha, uint64_t beta_1, uint64_t beta_2) const {

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate DDCF keys"));
    Logger::DebugLog(LOC, "Alpha: " + ToString(alpha));
    Logger::DebugLog(LOC, "Beta 1: " + ToString(beta_1));
    Logger::DebugLog(LOC, "Beta 2: " + ToString(beta_2));
#endif

    uint64_t e = params_.GetOutputBitsize();

    std::array<DdcfKey, 2>      keys     = {DdcfKey(0, params_), DdcfKey(1, params_)};
    std::pair<DdcfKey, DdcfKey> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    // Calculate beta
    uint64_t                                      beta     = Mod2N(beta_1 - beta_2, e);
    std::pair<fss::dcf::DcfKey, fss::dcf::DcfKey> dcf_keys = gen_.GenerateKeys(alpha, beta);

    // Set DDCF keys for each party
    key_pair.first.dcf_key  = std::move(dcf_keys.first);
    key_pair.second.dcf_key = std::move(dcf_keys.second);

    // Generate share of beta_2
    key_pair.first.mask  = Mod2N(GlobalRng::Rand<uint64_t>(), e);
    key_pair.second.mask = Mod2N(beta_2 - key_pair.first.mask, e);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    key_pair.first.PrintKey();
    key_pair.second.PrintKey();
#endif

    // Return the generated keys as a pair.
    return key_pair;
}

DdcfEvaluator::DdcfEvaluator(const DdcfParameters &params)
    : params_(params), eval_(params.GetParameters()) {
}

uint64_t DdcfEvaluator::EvaluateAt(const DdcfKey &key, uint64_t x) const {
    uint64_t party_id = key.dcf_key.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate DDCF key"));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
    Logger::DebugLog(LOC, party_str + " x: " + ToString(x));
#endif

    uint64_t output = eval_.EvaluateAt(key.dcf_key, x);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " Output: " + ToString(output));
#endif

    output = Mod2N(output + key.mask, params_.GetOutputBitsize());
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " Output after mask: " + ToString(output));
#endif

    return output;
}

}    // namespace proto
}    // namespace ringoa
