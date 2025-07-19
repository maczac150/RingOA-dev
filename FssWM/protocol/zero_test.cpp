#include "zero_test.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace proto {

void ZeroTestParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[ZeroTest Parameters] " + GetParametersInfo());
}

ZeroTestKey::ZeroTestKey(const uint64_t id, const ZeroTestParameters &params)
    : dpf_key(id, params.GetParameters()), shr_in(0),
      params_(params), serialized_size_(CalculateSerializedSize()) {
}

size_t ZeroTestKey::CalculateSerializedSize() const {
    size_t size = 0;

    size += dpf_key.GetSerializedSize();
    size += sizeof(shr_in);
    return size;
}

void ZeroTestKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing ZeroTest key");
#endif

    // Serialize the DPF key
    std::vector<uint8_t> key_buffer;
    dpf_key.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the shared random values
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&shr_in), reinterpret_cast<const uint8_t *>(&shr_in) + sizeof(shr_in));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void ZeroTestKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing ZeroTest key");
#endif
    size_t offset = 0;

    // Deserialize the DPF key
    size_t key_size = dpf_key.GetSerializedSize();
    dpf_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the shared random values
    std::memcpy(&shr_in, buffer.data() + offset, sizeof(shr_in));
}

void ZeroTestKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("ZeroTest Key [Party " + ToString(dpf_key.party_id) + "]"));
        dpf_key.PrintKey(true);
        Logger::DebugLog(LOC, "shr_in: " + ToString(shr_in));
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "ZeroTest Key [Party " + ToString(dpf_key.party_id) + "]");
        dpf_key.PrintKey(false);
        Logger::DebugLog(LOC, "shr_in: " + ToString(shr_in));
    }
#endif
}

ZeroTestKeyGenerator::ZeroTestKeyGenerator(
    const ZeroTestParameters   &params,
    sharing::AdditiveSharing2P &ss_in,
    sharing::AdditiveSharing2P &ss_out)
    : params_(params), gen_(params.GetParameters()), ss_in_(ss_in), ss_out_(ss_out) {
}

std::pair<ZeroTestKey, ZeroTestKey> ZeroTestKeyGenerator::GenerateKeys() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generating ZeroTest keys");
#endif

    std::array<ZeroTestKey, 2>          keys     = {ZeroTestKey(0, params_), ZeroTestKey(1, params_)};
    std::pair<ZeroTestKey, ZeroTestKey> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    // Generate random inputs for the keys
    uint64_t r_in = ss_in_.GenerateRandomValue();

    // Generate DPF keys
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> dpf_keys = gen_.GenerateKeys(r_in, 1);

    // Set DPF keys for each party
    key_pair.first.dpf_key  = std::move(dpf_keys.first);
    key_pair.second.dpf_key = std::move(dpf_keys.second);

    // Generate shared random values
    std::pair<uint64_t, uint64_t> r_in_sh = ss_in_.Share(r_in);
    key_pair.first.shr_in                 = r_in_sh.first;
    key_pair.second.shr_in                = r_in_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "r_in: " + ToString(r_in));
    key_pair.first.PrintKey();
    key_pair.second.PrintKey();
#endif

    // Return the generated keys as a pair.
    return key_pair;
}

ZeroTestEvaluator::ZeroTestEvaluator(
    const ZeroTestParameters   &params,
    sharing::AdditiveSharing2P &ss_in,
    sharing::AdditiveSharing2P &ss_out)
    : params_(params), eval_(params.GetParameters()), ss_in_(ss_in), ss_out_(ss_out) {
}

uint64_t ZeroTestEvaluator::EvaluateSharedInput(osuCrypto::Channel &chl, const ZeroTestKey &key, const uint64_t x) const {
    uint64_t party_id = key.dpf_key.party_id;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating ZeroTest protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " x: " + ToString(x));
#endif

    if (party_id == 0) {
        // Reconstruct masked inputs
        uint64_t masked_x_0, masked_x_1, masked_x;
        ss_in_.EvaluateAdd(x, key.shr_in, masked_x_0);
        ss_in_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_0: " + ToString(masked_x_0) + ", " + ToString(masked_x_1));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x));
#endif
        return EvaluateMaskedInput(key, masked_x);
    } else {
        // Reconstruct masked inputs
        uint64_t masked_x_0, masked_x_1, masked_x;
        ss_in_.EvaluateAdd(x, key.shr_in, masked_x_1);
        ss_in_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_1: " + ToString(masked_x_1));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x));
#endif
        return EvaluateMaskedInput(key, masked_x);
    }
}

uint64_t ZeroTestEvaluator::EvaluateMaskedInput(const ZeroTestKey &key, const uint64_t x) const {
    uint64_t party_id = key.dpf_key.party_id;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating ZeroTest protocol with masked inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " x: " + ToString(x));
#endif

    uint64_t output = eval_.EvaluateAt(key.dpf_key, x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " output: " + ToString(output));
#endif

    return output;
}

}    // namespace proto
}    // namespace fsswm
