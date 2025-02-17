#include "obliv_select.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace wm {

void OblivSelectParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Obliv Select Parameters]" + GetParametersInfo());
}

OblivSelectKey::OblivSelectKey(const uint32_t id, const OblivSelectParameters &params)
    : key0(id, params.GetParameters()), key1(id, params.GetParameters()), shr_r0(0), shr_r1(0), params_(params), serialized_size_(GetSerializedSize()) {
}

size_t OblivSelectKey::CalculateSerializedSize() const {
    return key0.GetSerializedSize() + key1.GetSerializedSize() + sizeof(shr_r0) + sizeof(shr_r1);
}

void OblivSelectKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing OblivSelectKey");
#endif

    // Serialize the DPF keys
    std::vector<uint8_t> key_buffer;
    key0.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    key_buffer.clear();
    key1.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the random shares
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&shr_r0), reinterpret_cast<const uint8_t *>(&shr_r0) + sizeof(shr_r0));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&shr_r1), reinterpret_cast<const uint8_t *>(&shr_r1) + sizeof(shr_r1));
}

void OblivSelectKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing OblivSelectKey");
#endif
    size_t offset = 0;

    // Deserialize the DPF keys
    size_t key_size = key0.CalculateSerializedSize();
    key0.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;
    key_size = key1.CalculateSerializedSize();
    key1.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the random shares
    std::memcpy(&shr_r0, buffer.data() + offset, sizeof(shr_r0));
    offset += sizeof(shr_r0);
    std::memcpy(&shr_r1, buffer.data() + offset, sizeof(shr_r1));
}

void OblivSelectKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("OblivSelect Key"));
        key0.PrintKey(true);
        key1.PrintKey(true);
        Logger::DebugLog(LOC, "shr_r0: " + std::to_string(shr_r0));
        Logger::DebugLog(LOC, "shr_r1: " + std::to_string(shr_r1));
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "OblivSelect Key");
        key0.PrintKey(false);
        key1.PrintKey(false);
        Logger::DebugLog(LOC, "shr_r0: " + std::to_string(shr_r0));
        Logger::DebugLog(LOC, "shr_r1: " + std::to_string(shr_r1));
    }
#endif
}

OblivSelectKeyGenerator::OblivSelectKeyGenerator(const OblivSelectParameters &params, sharing::ReplicatedSharing3P &rss, sharing::AdditiveSharing2P &ass)
    : params_(params), gen_(params.GetParameters()), rss_(rss), ass_(ass) {
}

std::array<OblivSelectKey, 3> OblivSelectKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<OblivSelectKey, 3> keys = {OblivSelectKey(0, params_), OblivSelectKey(1, params_), OblivSelectKey(2, params_)};

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generate OblivSelect Keys");
#endif

    // Sample the random numbers
    uint32_t                      r_0    = rss_.GenerateRandomValue();
    uint32_t                      r_1    = rss_.GenerateRandomValue();
    uint32_t                      r_2    = rss_.GenerateRandomValue();
    std::pair<uint32_t, uint32_t> r_0_sh = ass_.Share(r_0);
    std::pair<uint32_t, uint32_t> r_1_sh = ass_.Share(r_1);
    std::pair<uint32_t, uint32_t> r_2_sh = ass_.Share(r_2);

    // Generate the DPF keys
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> key_0 = gen_.GenerateKeys(r_0, 1);
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> key_1 = gen_.GenerateKeys(r_1, 1);
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> key_2 = gen_.GenerateKeys(r_2, 1);

    // Set the key and shared value
    keys[0].key0   = std::move(key_0.first);
    keys[0].key1   = std::move(key_1.second);
    keys[0].shr_r0 = r_0_sh.first;
    keys[0].shr_r1 = r_1_sh.second;
    keys[1].key0   = std::move(key_2.first);
    keys[1].key1   = std::move(key_0.second);
    keys[1].shr_r0 = r_2_sh.first;
    keys[1].shr_r1 = r_0_sh.second;
    keys[2].key0   = std::move(key_1.first);
    keys[2].key1   = std::move(key_2.second);
    keys[2].shr_r0 = r_1_sh.first;
    keys[2].shr_r1 = r_2_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "r_0: " + std::to_string(r_0));
    Logger::DebugLog(LOC, "r_1: " + std::to_string(r_1));
    Logger::DebugLog(LOC, "r_2: " + std::to_string(r_2));
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    return keys;
}

OblivSelectEvaluator::OblivSelectEvaluator(const OblivSelectParameters &params, sharing::ReplicatedSharing3P &rss)
    : params_(params), eval_(params.GetParameters()), rss_(rss) {
}

uint32_t OblivSelectEvaluator::Evaluate(sharing::Channels &chls, const OblivSelectKey &key, const uint32_t index) const {
}

}    // namespace wm
}    // namespace fsswm
