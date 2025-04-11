#include "fssfmi.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/plain_wm.h"

namespace {

void ReplaceOnes(std::vector<std::vector<uint32_t>> &matrix, uint32_t new_value) {
    for (auto &row : matrix) {
        for (auto &cell : row) {
            if (cell == 1) {
                cell = new_value;
            }
        }
    }
}

}    // namespace

namespace fsswm {
namespace fm_index {

void FssFMIParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[FssFMI Parameters]" + GetParametersInfo());
}

FssFMIKey::FssFMIKey(const uint32_t id, const FssFMIParameters &params)
    : num_wm_keys(params.GetQuerySize()),
      num_zt_keys(params.GetQuerySize()),
      params_(params) {
    wm_keys.reserve(num_wm_keys);
    zt_keys.reserve(num_zt_keys);
    for (uint32_t i = 0; i < num_wm_keys; ++i) {
        wm_keys.emplace_back(wm::FssWMKey(id, params.GetFssWMParameters()));
    }
    for (uint32_t i = 0; i < num_zt_keys; ++i) {
        zt_keys.emplace_back(ZeroTestKey(id, params.GetZeroTestParameters()));
    }
}

void FssFMIKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing FssFMIKey");
#endif

    // Serialize the number of WM keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_wm_keys), reinterpret_cast<const uint8_t *>(&num_wm_keys) + sizeof(num_wm_keys));

    // Serialize the number of ZT keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_zt_keys), reinterpret_cast<const uint8_t *>(&num_zt_keys) + sizeof(num_zt_keys));

    // Serialize the WM keys
    for (const auto &wm_key : wm_keys) {
        std::vector<uint8_t> key_buffer;
        wm_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    // Serialize the ZT keys
    for (const auto &zt_key : zt_keys) {
        std::vector<uint8_t> key_buffer;
        zt_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }
}

void FssFMIKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing FssFMIKey");
#endif
    size_t offset = 0;

    // Deserialize the number of WM keys
    std::memcpy(&num_wm_keys, buffer.data() + offset, sizeof(num_wm_keys));
    offset += sizeof(num_wm_keys);

    // Deserialize the number of ZT keys
    std::memcpy(&num_zt_keys, buffer.data() + offset, sizeof(num_zt_keys));
    offset += sizeof(num_zt_keys);

    // Deserialize the WM keys
    for (auto &wm_key : wm_keys) {
        size_t key_size = wm_key.GetSerializedSize();
        wm_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }

    // Deserialize the ZT keys
    for (auto &zt_key : zt_keys) {
        size_t key_size = zt_key.GetSerializedSize();
        zt_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
}

void FssFMIKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("FssFMI Key"));
    for (const auto &wm_key : wm_keys) {
        wm_key.PrintKey(detailed);
    }
    for (const auto &zt_key : zt_keys) {
        zt_key.PrintKey(detailed);
    }
}

FssFMIKeyGenerator::FssFMIKeyGenerator(
    const FssFMIParameters             &params,
    sharing::AdditiveSharing2P         &ass,
    sharing::BinarySharing2P           &bss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      wm_gen_(params.GetFssWMParameters(), ass, bss, brss),
      zt_gen_(params.GetZeroTestParameters(), ass, bss),
      brss_(brss) {
}

std::array<std::pair<sharing::RepShareMat, sharing::RepShareMat>, 3> FssFMIKeyGenerator::GenerateDatabaseShare(const wm::FMIndex &fm) {
    return wm_gen_.GenerateDatabaseShare(fm);
}

std::array<sharing::RepShareMat, 3> FssFMIKeyGenerator::GenerateQueryShare(const wm::FMIndex &fm, std::string &query) {
    std::vector<std::vector<uint32_t>> query_bv = fm.ConvertToBitMatrix(query);
    ReplaceOnes(query_bv, Pow(2, params_.GetDatabaseBitSize()) - 1);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query bitvec: " + ToStringMat(query_bv));
#endif
    return brss_.ShareLocal(query_bv);
}

std::array<FssFMIKey, 3> FssFMIKeyGenerator::GenerateKeys() const {
    // Initialize keys
    std::array<FssFMIKey, 3> keys = {
        FssFMIKey(0, params_),
        FssFMIKey(1, params_),
        FssFMIKey(2, params_)};

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate FssWM keys"));
#endif

    for (uint32_t i = 0; i < keys[0].num_wm_keys; ++i) {
        // Generate the FssWMKey keys
        std::array<wm::FssWMKey, 3> wm_key = wm_gen_.GenerateKeys();

        // Set the FssWMKey keys
        keys[0].wm_keys[i] = std::move(wm_key[0]);
        keys[1].wm_keys[i] = std::move(wm_key[1]);
        keys[2].wm_keys[i] = std::move(wm_key[2]);
    }

    for (uint32_t i = 0; i < keys[0].num_zt_keys; ++i) {
        // Generate the ZeroTestKey
        std::array<ZeroTestKey, 3> zt_key = zt_gen_.GenerateKeys();

        // Set the ZeroTestKey keys
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

FssFMIEvaluator::FssFMIEvaluator(const FssFMIParameters             &params,
                                 sharing::ReplicatedSharing3P       &rss,
                                 sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      wm_eval_(params.GetFssWMParameters(), rss, brss),
      zt_eval_(params.GetZeroTestParameters(), rss, brss),
      rss_(rss),
      brss_(brss) {
}

void FssFMIEvaluator::EvaluateLPM(sharing::Channels          &chls,
                                  const FssFMIKey            &key,
                                  const sharing::RepShareMat &wm_table0,
                                  const sharing::RepShareMat &wm_table1,
                                  const sharing::RepShareMat &query,
                                  sharing::RepShareVec       &result) const {

    uint32_t d        = params_.GetDatabaseBitSize();
    uint32_t nu       = params_.GetFssWMParameters().GetOSParameters().GetParameters().GetTerminateBitsize();
    uint32_t ds       = params_.GetDatabaseSize();
    uint32_t qs       = params_.GetQuerySize();
    uint32_t sigma    = params_.GetSigma();
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssFMI key"));
    Logger::DebugLog(LOC, "Database bit size: " + std::to_string(d));
    Logger::DebugLog(LOC, "Database size: " + std::to_string(ds));
    Logger::DebugLog(LOC, "Query size: " + std::to_string(qs));
    Logger::DebugLog(LOC, "Sigma: " + std::to_string(sigma));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
#endif

    sharing::RepShare    f_sh(0, 0), g_sh(0, 0);
    sharing::RepShare    f_next_sh(0, 0), g_next_sh(0, 0);
    sharing::RepShareVec interval_sh(qs);

    if (party_id == 0) {
        g_sh.data[0] = wm_table0.RowView(0).num_shares() - 1;
    } else if (party_id == 1) {
        g_sh.data[1] = wm_table1.RowView(0).num_shares() - 1;
    }

    std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
    for (uint32_t i = 0; i < qs; ++i) {
        wm_eval_.EvaluateRankCF(chls, uv_prev, uv_next, key.wm_keys[i], wm_table0, wm_table1, query.RowView(i), f_sh, f_next_sh);
        wm_eval_.EvaluateRankCF(chls, uv_prev, uv_next, key.wm_keys[i], wm_table0, wm_table1, query.RowView(i), g_sh, g_next_sh);
        f_sh = f_next_sh;
        g_sh = g_next_sh;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint32_t f, g;
        brss_.Open(chls, f_sh, f);
        brss_.Open(chls, g_sh, g);
        Logger::InfoLog(LOC, party_str + "f(" + std::to_string(i) + "): " + std::to_string(f));
        Logger::InfoLog(LOC, party_str + "g(" + std::to_string(i) + "): " + std::to_string(g));
#endif
        sharing::RepShare fg_xor_sh;
        brss_.EvaluateXor(f_sh, g_sh, fg_xor_sh);
        interval_sh.Set(i, fg_xor_sh);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    sharing::UIntVec interval;
    brss_.Open(chls, interval_sh, interval);
    Logger::DebugLog(LOC, party_str + "Interval: " + ToString(interval));
#endif

    zt_eval_.EvaluateBinary(chls, key.zt_keys, interval_sh, result);
}

}    // namespace fm_index
}    // namespace fsswm
