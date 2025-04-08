#include "fsswm.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"
#include "plain_wm.h"

namespace fsswm {
namespace wm {

using fsswm::sharing::BinaryReplicatedSharing3P;

void FssWMParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[FssWM Parameters]" + GetParametersInfo());
}

FssWMKey::FssWMKey(const uint32_t id, const FssWMParameters &params)
    : num_os_keys(params.GetSigma()), params_(params) {
    os_keys.reserve(num_os_keys);
    for (uint32_t i = 0; i < num_os_keys; ++i) {
        os_keys.emplace_back(OblivSelectKey(id, params.GetOSParameters()));
    }
}

void FssWMKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing FssWMKey");
#endif

    // Serialize the number of OS keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_os_keys), reinterpret_cast<const uint8_t *>(&num_os_keys) + sizeof(num_os_keys));

    // Serialize the OS keys
    for (const auto &os_key : os_keys) {
        std::vector<uint8_t> key_buffer;
        os_key.Serialize(key_buffer);
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
    for (auto &os_key : os_keys) {
        size_t key_size = os_key.GetSerializedSize();
        os_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
}

void FssWMKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("FssWM Key"));
    Logger::DebugLog(LOC, "Number of OblivSelect Keys: " + std::to_string(num_os_keys));
    for (uint32_t i = 0; i < num_os_keys; ++i) {
        os_keys[i].PrintKey(detailed);
    }
}

FssWMKeyGenerator::FssWMKeyGenerator(
    const FssWMParameters &             params,
    sharing::AdditiveSharing2P &        ass,
    sharing::BinarySharing2P &          bss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      os_gen_(params.GetOSParameters(), ass, bss),
      brss_(brss) {
}

std::array<std::pair<sharing::RepShareMat, sharing::RepShareMat>, 3> FssWMKeyGenerator::GenerateDatabaseShare(std::string &database) {
    FMIndex                                                              fm(params_.GetSigma(), database);
    std::vector<std::vector<uint32_t>>                                   rank0 = fm.GetRank0Tables();
    std::vector<std::vector<uint32_t>>                                   rank1 = fm.GetRank1Tables();
    std::array<std::pair<sharing::RepShareMat, sharing::RepShareMat>, 3> db_sh;

    std::array<sharing::RepShareMat, 3> rank0_sh = brss_.ShareLocal(rank0);
    std::array<sharing::RepShareMat, 3> rank1_sh = brss_.ShareLocal(rank1);

    for (size_t p = 0; p < sharing::kNumParties; ++p) {
        db_sh[p].first  = std::move(rank0_sh[p]);
        db_sh[p].second = std::move(rank1_sh[p]);
    }
    return db_sh;
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
        std::array<OblivSelectKey, 3> os_key = os_gen_.GenerateKeys();

        // Set the OblivSelect keys
        keys[0].os_keys[i] = std::move(os_key[0]);
        keys[1].os_keys[i] = std::move(os_key[1]);
        keys[2].os_keys[i] = std::move(os_key[2]);
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
    const FssWMParameters &             params,
    sharing::ReplicatedSharing3P &      rss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      os_eval_(params.GetOSParameters(), rss, brss),
      rss_(rss), brss_(brss) {
}

void FssWMEvaluator::EvaluateRankCF(sharing::Channels &         chls,
                                    std::vector<block> &        uv_prev,
                                    std::vector<block> &        uv_next,
                                    const FssWMKey &            key,
                                    const sharing::RepShareMat &wm_table0,
                                    const sharing::RepShareMat &wm_table1,
                                    const sharing::RepShareVec &char_sh,
                                    sharing::RepShare &         position_sh,
                                    sharing::RepShare &         result) const {
    uint32_t d        = params_.GetDatabaseBitSize();
    uint32_t ds       = params_.GetDatabaseSize();
    uint32_t sigma    = params_.GetSigma();
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssWM key"));
    Logger::DebugLog(LOC, "Database bit size: " + std::to_string(d));
    Logger::DebugLog(LOC, "Database size: " + std::to_string(ds));
    Logger::DebugLog(LOC, "Sigma: " + std::to_string(sigma));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
#endif

    for (uint32_t i = 0; i < sigma; ++i) {
        sharing::RepShare rank0_sh, rank1_sh;
        os_eval_.EvaluateBinary(chls, uv_prev, uv_next, key.os_keys[i], wm_table0.RowView(i), wm_table1.RowView(i), position_sh, rank0_sh, rank1_sh);
        brss_.EvaluateSelect(chls, rank0_sh, rank1_sh, char_sh.At(i), position_sh);
    }
    result = position_sh;
}

}    // namespace wm
}    // namespace fsswm
