#include "fsswm.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"
#include "plain_wm.h"

namespace fsswm {
namespace wm {

void FssWMParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[FssWM Parameters]" + GetParametersInfo());
}

FssWMKey::FssWMKey(const uint32_t id, const FssWMParameters &params)
    : num_os_keys(params.GetQuerySize() * params.GetSigma()), num_zt_keys(params.GetQuerySize()), params_(params) {
    os_f_keys.reserve(num_os_keys);
    os_g_keys.reserve(num_os_keys);
    for (uint32_t i = 0; i < num_os_keys; ++i) {
        os_f_keys.emplace_back(OblivSelectKey(id, params.GetOSParameters()));
        os_g_keys.emplace_back(OblivSelectKey(id, params.GetOSParameters()));
    }
    zt_keys.reserve(num_zt_keys);
    for (uint32_t i = 0; i < num_zt_keys; ++i) {
        zt_keys.emplace_back(ZeroTestKey(id, params.GetZTParameters()));
    }
}

void FssWMKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing FssWMKey");
#endif

    // Serialize the number of OS keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_os_keys), reinterpret_cast<const uint8_t *>(&num_os_keys) + sizeof(num_os_keys));

    // Serialize the OS keys
    for (const auto &os_key : os_f_keys) {
        std::vector<uint8_t> key_buffer;
        os_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    for (const auto &os_key : os_g_keys) {
        std::vector<uint8_t> key_buffer;
        os_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    // Serialize the number of ZT keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_zt_keys), reinterpret_cast<const uint8_t *>(&num_zt_keys) + sizeof(num_zt_keys));

    // Serialize the ZT keys
    for (const auto &zt_key : zt_keys) {
        std::vector<uint8_t> key_buffer;
        zt_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }
}

void FssWMKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing FssWMKey");
#endif
    size_t offset = 0;

    // Deserialize the number of OS keys
    std::memcpy(&num_os_keys, buffer.data() + offset, sizeof(num_os_keys));
    offset += sizeof(num_os_keys);

    // Deserialize the OS keys
    for (auto &os_key : os_f_keys) {
        size_t key_size = os_key.GetSerializedSize();
        os_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }

    for (auto &os_key : os_g_keys) {
        size_t key_size = os_key.GetSerializedSize();
        os_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }

    // Deserialize the number of ZT keys
    std::memcpy(&num_zt_keys, buffer.data() + offset, sizeof(num_zt_keys));
    offset += sizeof(num_zt_keys);

    // Deserialize the ZT keys
    for (auto &zt_key : zt_keys) {
        size_t key_size = zt_key.GetSerializedSize();
        zt_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
}

void FssWMKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("FssWM Key"));
    Logger::DebugLog(LOC, "Number of OblivSelect Keys: " + std::to_string(num_os_keys));
    for (uint32_t i = 0; i < num_os_keys; ++i) {
        os_f_keys[i].PrintKey(detailed);
        os_g_keys[i].PrintKey(detailed);
    }
    Logger::DebugLog(LOC, "Number of ZeroTest Keys: " + std::to_string(num_zt_keys));
    for (uint32_t i = 0; i < num_zt_keys; ++i) {
        zt_keys[i].PrintKey(detailed);
    }
}

FssWMKeyGenerator::FssWMKeyGenerator(
    const FssWMParameters      &params,
    sharing::AdditiveSharing2P &ass,
    sharing::BinarySharing2P   &bss)
    : params_(params),
      os_gen_(params.GetOSParameters(), ass, bss),
      zt_gen_(params.GetZTParameters(), ass, bss),
      ass_(ass), bss_(bss) {
}

std::vector<sharing::SharesPair> FssWMKeyGenerator::GenerateDatabaseShare(std::string &database) {
}

sharing::SharesPair FssWMKeyGenerator::GenerateQueryShare(std::string &query) {
}

std::array<FssWMKey, 3> FssWMKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<FssWMKey, 3> keys = {
        FssWMKey(0, params_),
        FssWMKey(1, params_),
        FssWMKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate FssWM keys"));
#endif

    for (uint32_t i = 0; i < keys[0].num_os_keys; ++i) {
        // Generate the OblivSelect keys
        std::array<OblivSelectKey, 3> os_f_key = os_gen_.GenerateKeys();
        std::array<OblivSelectKey, 3> os_g_key = os_gen_.GenerateKeys();

        // Set the OblivSelect keys
        keys[0].os_f_keys[i] = std::move(os_f_key[0]);
        keys[1].os_f_keys[i] = std::move(os_f_key[1]);
        keys[2].os_f_keys[i] = std::move(os_f_key[2]);
        keys[0].os_g_keys[i] = std::move(os_g_key[0]);
        keys[1].os_g_keys[i] = std::move(os_g_key[1]);
        keys[2].os_g_keys[i] = std::move(os_g_key[2]);
    }

    for (uint32_t i = 0; i < keys[0].num_zt_keys; ++i) {
        // Generate the ZeroTest keys
        std::array<ZeroTestKey, 3> zt_key = zt_gen_.GenerateKeys();

        // Set the ZeroTest keys
        keys[0].zt_keys[i] = std::move(zt_key[0]);
        keys[1].zt_keys[i] = std::move(zt_key[1]);
        keys[2].zt_keys[i] = std::move(zt_key[2]);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "FssWM keys generated");
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    // Return the keys
    return keys;
}

FssWMEvaluator::FssWMEvaluator(
    const FssWMParameters              &params,
    sharing::ReplicatedSharing3P       &rss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      os_eval_(params.GetOSParameters(), rss, brss),
      zt_eval_(params.GetZTParameters(), rss, brss),
      rss_(rss), brss_(brss) {
}

void FssWMEvaluator::Evaluate(sharing::Channels                      &chls,
                              const FssWMKey                         &key,
                              const std::vector<sharing::SharesPair> &wm_table,
                              const sharing::SharesPair              &query,
                              sharing::SharesPair                    &result) const {
    uint32_t d        = params_.GetDatabaseBitSize();
    uint32_t ds       = params_.GetDatabaseSize();
    uint32_t qs       = params_.GetQuerySize();
    uint32_t sigma    = params_.GetSigma();
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssWM key"));
    Logger::DebugLog(LOC, "Database bit size: " + std::to_string(d));
    Logger::DebugLog(LOC, "Database size: " + std::to_string(ds));
    Logger::DebugLog(LOC, "Query size: " + std::to_string(qs));
    Logger::DebugLog(LOC, "Sigma: " + std::to_string(sigma));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
#endif
}

}    // namespace wm
}    // namespace fsswm
