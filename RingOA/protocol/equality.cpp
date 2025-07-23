#include "equality.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace proto {

void EqualityParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Equality Parameters] " + GetParametersInfo());
}

EqualityKey::EqualityKey(const uint64_t id, const EqualityParameters &params)
    : dpf_key(id, params.GetParameters()),
      shr1_in(0), shr2_in(0),
      params_(params), serialized_size_(CalculateSerializedSize()) {
}

size_t EqualityKey::CalculateSerializedSize() const {
    size_t size = 0;

    size += dpf_key.GetSerializedSize();
    size += sizeof(shr1_in);
    size += sizeof(shr2_in);
    return size;
}

void EqualityKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing Equality key");
#endif

    // Serialize the DPF key
    std::vector<uint8_t> key_buffer;
    dpf_key.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the shared random values
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&shr1_in), reinterpret_cast<const uint8_t *>(&shr1_in) + sizeof(shr1_in));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&shr2_in), reinterpret_cast<const uint8_t *>(&shr2_in) + sizeof(shr2_in));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void EqualityKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing Equality key");
#endif
    size_t offset = 0;

    // Deserialize the DPF key
    size_t key_size = dpf_key.GetSerializedSize();
    dpf_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the shared random values
    std::memcpy(&shr1_in, buffer.data() + offset, sizeof(shr1_in));
    offset += sizeof(shr1_in);
    std::memcpy(&shr2_in, buffer.data() + offset, sizeof(shr2_in));
}

void EqualityKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("Equality Key [Party " + ToString(dpf_key.party_id) + "]"));
        dpf_key.PrintKey(true);
        Logger::DebugLog(LOC, "shr1_in: " + ToString(shr1_in));
        Logger::DebugLog(LOC, "shr2_in: " + ToString(shr2_in));
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "Equality Key [Party " + ToString(dpf_key.party_id) + "]");
        dpf_key.PrintKey(false);
        Logger::DebugLog(LOC, "shr1_in: " + ToString(shr1_in));
        Logger::DebugLog(LOC, "shr2_in: " + ToString(shr2_in));
    }
#endif
}

EqualityKeyGenerator::EqualityKeyGenerator(
    const EqualityParameters   &params,
    sharing::AdditiveSharing2P &ss_in,
    sharing::AdditiveSharing2P &ss_out)
    : params_(params), gen_(params.GetParameters()), ss_in_(ss_in), ss_out_(ss_out) {
}

std::pair<EqualityKey, EqualityKey> EqualityKeyGenerator::GenerateKeys() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generating Equality keys");
#endif

    uint64_t                            n        = params_.GetInputBitsize();
    std::array<EqualityKey, 2>          keys     = {EqualityKey(0, params_), EqualityKey(1, params_)};
    std::pair<EqualityKey, EqualityKey> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    // Generate random inputs for the keys
    uint64_t r1_in = ss_in_.GenerateRandomValue();
    uint64_t r2_in = ss_in_.GenerateRandomValue();

    // Generate DPF keys
    uint64_t                                      alpha    = Mod(r1_in - r2_in, n);
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> dpf_keys = gen_.GenerateKeys(alpha, 1);

    // Set DPF keys for each party
    key_pair.first.dpf_key  = std::move(dpf_keys.first);
    key_pair.second.dpf_key = std::move(dpf_keys.second);

    // Generate shared random values
    std::pair<uint64_t, uint64_t> r1_in_sh = ss_in_.Share(r1_in);
    std::pair<uint64_t, uint64_t> r2_in_sh = ss_in_.Share(r2_in);

    key_pair.first.shr1_in  = r1_in_sh.first;
    key_pair.second.shr1_in = r1_in_sh.second;
    key_pair.first.shr2_in  = r2_in_sh.first;
    key_pair.second.shr2_in = r2_in_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Random inputs: r1_in = " + ToString(r1_in) + ", r2_in = " + ToString(r2_in));
    Logger::DebugLog(LOC, "alpha: " + ToString(alpha));
    key_pair.first.PrintKey();
    key_pair.second.PrintKey();
#endif

    // Return the generated keys as a pair.
    return key_pair;
}

EqualityEvaluator::EqualityEvaluator(
    const EqualityParameters   &params,
    sharing::AdditiveSharing2P &ss_in,
    sharing::AdditiveSharing2P &ss_out)
    : params_(params), eval_(params.GetParameters()), ss_in_(ss_in), ss_out_(ss_out) {
}

uint64_t EqualityEvaluator::EvaluateSharedInput(osuCrypto::Channel &chl, const EqualityKey &key, const uint64_t x1, const uint64_t x2) const {
    uint64_t party_id = key.dpf_key.party_id;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating Equality protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " x1: " + ToString(x1) + ", x2: " + ToString(x2));
#endif

    if (party_id == 0) {
        // Reconstruct masked inputs
        std::array<uint64_t, 2> masked_x_0, masked_x_1, masked_x;
        ss_in_.EvaluateAdd({x1, x2}, {key.shr1_in, key.shr2_in}, masked_x_0);
        ss_in_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_0: " + ToString(masked_x_0[0]) + ", " + ToString(masked_x_0[1]));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x[0]) + ", " + ToString(masked_x[1]));
#endif
        return EvaluateMaskedInput(key, masked_x[0], masked_x[1]);
    } else {
        // Reconstruct masked inputs
        std::array<uint64_t, 2> masked_x_0, masked_x_1, masked_x;
        ss_in_.EvaluateAdd({x1, x2}, {key.shr1_in, key.shr2_in}, masked_x_1);
        ss_in_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_1: " + ToString(masked_x_1[0]) + ", " + ToString(masked_x_1[1]));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x[0]) + ", " + ToString(masked_x[1]));
#endif
        return EvaluateMaskedInput(key, masked_x[0], masked_x[1]);
    }
}

uint64_t EqualityEvaluator::EvaluateMaskedInput(const EqualityKey &key, const uint64_t x1, const uint64_t x2) const {
    uint64_t party_id = key.dpf_key.party_id;
    uint64_t n        = params_.GetInputBitsize();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating Equality protocol with masked inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " x1: " + ToString(x1) + ", x2: " + ToString(x2));
#endif

    uint64_t alpha  = Mod(x1 - x2, n);
    uint64_t output = eval_.EvaluateAt(key.dpf_key, alpha);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " alpha: " + ToString(alpha) + ", output: " + ToString(output));
#endif

    return output;
}

}    // namespace proto
}    // namespace fsswm
