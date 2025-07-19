#include "integer_comparison.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace proto {

void IntegerComparisonParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[IntegerComparison Parameters] " + GetParametersInfo());
}

IntegerComparisonKey::IntegerComparisonKey(const uint64_t id, const IntegerComparisonParameters &params)
    : ddcf_key(id, params.GetParameters()),
      shr1_in(0), shr2_in(0),
      params_(params), serialized_size_(CalculateSerializedSize()) {
}

size_t IntegerComparisonKey::CalculateSerializedSize() const {
    size_t size = 0;

    size += ddcf_key.GetSerializedSize();
    size += sizeof(shr1_in);
    size += sizeof(shr2_in);
    return size;
}

void IntegerComparisonKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing IntegerComparison key");
#endif

    // Serialize the DDCF key
    std::vector<uint8_t> key_buffer;
    ddcf_key.Serialize(key_buffer);
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

void IntegerComparisonKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing IntegerComparison key");
#endif
    size_t offset = 0;

    // Deserialize the DDCF key
    size_t key_size = ddcf_key.GetSerializedSize();
    ddcf_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the shared random values
    std::memcpy(&shr1_in, buffer.data() + offset, sizeof(shr1_in));
    offset += sizeof(shr1_in);
    std::memcpy(&shr2_in, buffer.data() + offset, sizeof(shr2_in));
}

void IntegerComparisonKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("IntegerComparison Key [Party " + ToString(ddcf_key.dcf_key.party_id) + "]"));
        ddcf_key.PrintKey(true);
        Logger::DebugLog(LOC, "shr1_in: " + ToString(shr1_in));
        Logger::DebugLog(LOC, "shr2_in: " + ToString(shr2_in));
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "IntegerComparison Key [Party " + ToString(ddcf_key.dcf_key.party_id) + "]");
        ddcf_key.PrintKey(false);
        Logger::DebugLog(LOC, "shr1_in: " + ToString(shr1_in));
        Logger::DebugLog(LOC, "shr2_in: " + ToString(shr2_in));
    }
#endif
}

IntegerComparisonKeyGenerator::IntegerComparisonKeyGenerator(
    const IntegerComparisonParameters &params,
    sharing::AdditiveSharing2P        &ss_in,
    sharing::AdditiveSharing2P        &ss_out)
    : params_(params), gen_(params.GetParameters()), ss_in_(ss_in), ss_out_(ss_out) {
}

std::pair<IntegerComparisonKey, IntegerComparisonKey> IntegerComparisonKeyGenerator::GenerateKeys() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generating IntegerComparison keys");
#endif

    uint64_t n = params_.GetInputBitsize();

    std::array<IntegerComparisonKey, 2>                   keys     = {IntegerComparisonKey(0, params_), IntegerComparisonKey(1, params_)};
    std::pair<IntegerComparisonKey, IntegerComparisonKey> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    // Generate random inputs for the keys
    uint64_t r1_in = ss_in_.GenerateRandomValue();
    uint64_t r2_in = ss_in_.GenerateRandomValue();

    // Compute the random value r and alpha
    uint64_t r     = Mod(Pow(2, n) - (r1_in - r2_in), n);
    uint64_t alpha = GetLowerNBits(r, n - 1);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "r1_in: " + ToString(r1_in) + ", r2_in: " + ToString(r2_in));
    Logger::DebugLog(LOC, "r: " + ToString(r) + ", alpha: " + ToString(alpha));
#endif

    // Generate DDCF keys
    uint64_t                    msb_r     = GetMSB(r, n);
    uint64_t                    beta_1    = !msb_r;
    uint64_t                    beta_2    = msb_r;
    std::pair<DdcfKey, DdcfKey> ddcf_keys = gen_.GenerateKeys(alpha, beta_1, beta_2);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "beta_1: " + ToString(beta_1) + ", beta_2: " + ToString(beta_2));
#endif

    // Set DCF keys for each party
    key_pair.first.ddcf_key  = std::move(ddcf_keys.first);
    key_pair.second.ddcf_key = std::move(ddcf_keys.second);

    // Generate shared random values
    std::pair<uint64_t, uint64_t> r1_in_sh = ss_in_.Share(r1_in);
    std::pair<uint64_t, uint64_t> r2_in_sh = ss_in_.Share(r2_in);

    key_pair.first.shr1_in  = r1_in_sh.first;
    key_pair.second.shr1_in = r1_in_sh.second;
    key_pair.first.shr2_in  = r2_in_sh.first;
    key_pair.second.shr2_in = r2_in_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    key_pair.first.PrintKey();
    key_pair.second.PrintKey();
#endif

    // Return the generated keys as a pair.
    return key_pair;
}

IntegerComparisonEvaluator::IntegerComparisonEvaluator(
    const IntegerComparisonParameters &params,
    sharing::AdditiveSharing2P        &ss_in,
    sharing::AdditiveSharing2P        &ss_out)
    : params_(params), eval_(params.GetParameters()), ss_in_(ss_in), ss_out_(ss_out) {
}

uint64_t IntegerComparisonEvaluator::EvaluateSharedInput(osuCrypto::Channel &chl, const IntegerComparisonKey &key, const uint64_t x1, const uint64_t x2) const {
    uint64_t party_id = key.ddcf_key.dcf_key.party_id;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating Equality protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " x1: " + ToString(x1) + ", x2: " + ToString(x2));
#endif

    if (party_id == 0) {
        // Reconstruct masked inputs
        std::array<uint64_t, 2> masked_x_0, masked_x_1, masked_x;
        ss_in_.EvaluateSub({x1, x2}, {key.shr1_in, key.shr2_in}, masked_x_0);
        ss_in_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_0: " + ToString(masked_x_0[0]) + ", " + ToString(masked_x_0[1]));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x[0]) + ", " + ToString(masked_x[1]));
#endif
        return EvaluateMaskedInput(key, masked_x[0], masked_x[1]);
    } else {
        // Reconstruct masked inputs
        std::array<uint64_t, 2> masked_x_0, masked_x_1, masked_x;
        ss_in_.EvaluateSub({x1, x2}, {key.shr1_in, key.shr2_in}, masked_x_1);
        ss_in_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_1: " + ToString(masked_x_1[0]) + ", " + ToString(masked_x_1[1]));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x[0]) + ", " + ToString(masked_x[1]));
#endif
        return EvaluateMaskedInput(key, masked_x[0], masked_x[1]);
    }
}

uint64_t IntegerComparisonEvaluator::EvaluateMaskedInput(const IntegerComparisonKey &key, const uint64_t x1, const uint64_t x2) const {
    uint64_t party_id = key.ddcf_key.dcf_key.party_id;
    uint64_t n        = params_.GetInputBitsize();
    uint64_t e        = params_.GetOutputBitsize();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating Equality protocol with masked inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " x1: " + ToString(x1) + ", x2: " + ToString(x2));
#endif

    uint64_t z     = Mod(x1 - x2, n);
    uint64_t msb_z = GetMSB(z, n);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " z: " + ToString(z) + ", msb_z: " + ToString(msb_z));
#endif

    // Evaluate the DDCF key
    uint64_t alpha  = Mod(Pow(2, n - 1) - GetLowerNBits(z, n - 1) - 1, n - 1);
    uint64_t output = eval_.EvaluateAt(key.ddcf_key, alpha);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " alpha: " + ToString(alpha));
    Logger::DebugLog(LOC, party_str + " output: " + ToString(output));
#endif

    output = Mod(party_id - ((party_id * msb_z) + output - (2 * msb_z * output)), e);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " Final output: " + ToString(output));
#endif

    return output;
}

}    // namespace proto
}    // namespace fsswm
