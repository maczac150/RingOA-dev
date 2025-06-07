#include "fssfmi.h"

#include <cstring>

#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/plain_wm.h"

namespace {

void ReplaceOnes(std::vector<uint64_t> &vector, uint64_t new_value) {
    for (auto &cell : vector) {
        if (cell == 1) {
            cell = new_value;
        }
    }
}

}    // namespace

namespace fsswm {
namespace fm_index {

void FssFMIParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[FssFMI Parameters]" + GetParametersInfo());
}

FssFMIKey::FssFMIKey(const uint64_t id, const FssFMIParameters &params)
    : num_wm_keys(params.GetQuerySize()),
      num_zt_keys(params.GetQuerySize()),
      params_(params) {
    wm_f_keys.reserve(num_wm_keys);
    wm_g_keys.reserve(num_wm_keys);
    zt_keys.reserve(num_zt_keys);
    for (uint64_t i = 0; i < num_wm_keys; ++i) {
        wm_f_keys.emplace_back(wm::FssWMKey(id, params.GetFssWMParameters()));
        wm_g_keys.emplace_back(wm::FssWMKey(id, params.GetFssWMParameters()));
    }
    for (uint64_t i = 0; i < num_zt_keys; ++i) {
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
    for (const auto &wm_key : wm_f_keys) {
        std::vector<uint8_t> key_buffer;
        wm_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    for (const auto &wm_key : wm_g_keys) {
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
    for (auto &wm_key : wm_f_keys) {
        size_t key_size = wm_key.GetSerializedSize();
        wm_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }

    for (auto &wm_key : wm_g_keys) {
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
    for (const auto &wm_key : wm_f_keys) {
        wm_key.PrintKey(detailed);
    }
    for (const auto &wm_key : wm_g_keys) {
        wm_key.PrintKey(detailed);
    }
    for (const auto &zt_key : zt_keys) {
        zt_key.PrintKey(detailed);
    }
}

FssFMIKeyGenerator::FssFMIKeyGenerator(
    const FssFMIParameters             &params,
    sharing::BinarySharing2P           &bss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      wm_gen_(params.GetFssWMParameters(), bss, brss),
      zt_gen_(params.GetZeroTestParameters(), bss),
      brss_(brss) {
}

std::array<sharing::RepShareMatBlock, 3> FssFMIKeyGenerator::GenerateDatabaseBlockShare(const wm::FMIndex &fm) const {
    return wm_gen_.GenerateDatabaseBlockShare(fm);
}

std::array<sharing::RepShareMat64, 3> FssFMIKeyGenerator::GenerateDatabaseU64Share(const wm::FMIndex &fm) const {
    return wm_gen_.GenerateDatabaseU64Share(fm);
}

std::array<sharing::RepShareMat64, 3> FssFMIKeyGenerator::GenerateQueryShare(const wm::FMIndex &fm, std::string &query) const {
    std::vector<uint64_t> query_bv = fm.ConvertToBitMatrix(query);
    ReplaceOnes(query_bv, Pow(2, params_.GetDatabaseBitSize()) - 1);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query bitvec: " + ToStringMatrix(query_bv, params_.GetQuerySize(), fm.GetWaveletMatrix().GetSigma()));
#endif
    return brss_.ShareLocal(query_bv, params_.GetQuerySize(), fm.GetWaveletMatrix().GetSigma());
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

    for (uint64_t i = 0; i < keys[0].num_wm_keys; ++i) {
        // Generate the FssWMKey keys
        std::array<wm::FssWMKey, 3> wm_f_key = wm_gen_.GenerateKeys();
        std::array<wm::FssWMKey, 3> wm_g_key = wm_gen_.GenerateKeys();

        // Set the FssWMKey keys
        keys[0].wm_f_keys[i] = std::move(wm_f_key[0]);
        keys[1].wm_f_keys[i] = std::move(wm_f_key[1]);
        keys[2].wm_f_keys[i] = std::move(wm_f_key[2]);
        keys[0].wm_g_keys[i] = std::move(wm_g_key[0]);
        keys[1].wm_g_keys[i] = std::move(wm_g_key[1]);
        keys[2].wm_g_keys[i] = std::move(wm_g_key[2]);
    }

    for (uint64_t i = 0; i < keys[0].num_zt_keys; ++i) {
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
                                 sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      wm_eval_(params.GetFssWMParameters(), brss),
      zt_eval_(params.GetZeroTestParameters(), brss),
      brss_(brss) {
}

void FssFMIEvaluator::EvaluateLPM_SingleBitMask(Channels                        &chls,
                                                const FssFMIKey                 &key,
                                                const sharing::RepShareMatBlock &wm_tables,
                                                const sharing::RepShareMat64    &query,
                                                sharing::RepShareVec64          &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t qs       = params_.GetQuerySize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssFMI key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
    Logger::DebugLog(LOC, "Query size: " + ToString(qs));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    sharing::RepShare64    f_sh(0, 0), g_sh(0, 0);
    sharing::RepShare64    f_next_sh(0, 0), g_next_sh(0, 0);
    sharing::RepShareVec64 interval_sh(qs);

    if (party_id == 0) {
        g_sh.data[0] = wm_tables.RowView(0).Size() - 1;
    } else if (party_id == 1) {
        g_sh.data[1] = wm_tables.RowView(0).Size() - 1;
    }

    for (uint64_t i = 0; i < qs; ++i) {
        wm_eval_.EvaluateRankCF_SingleBitMask(chls, key.wm_f_keys[i], wm_tables, query.RowView(i), f_sh, f_next_sh);
        wm_eval_.EvaluateRankCF_SingleBitMask(chls, key.wm_g_keys[i], wm_tables, query.RowView(i), g_sh, g_next_sh);
        f_sh = f_next_sh;
        g_sh = g_next_sh;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint64_t f, g;
        brss_.Open(chls, f_sh, f);
        brss_.Open(chls, g_sh, g);
        Logger::InfoLog(LOC, party_str + "f(" + ToString(i) + "): " + ToString(f));
        Logger::InfoLog(LOC, party_str + "g(" + ToString(i) + "): " + ToString(g));
#endif
        sharing::RepShare64 fg_xor_sh;
        brss_.EvaluateXor(f_sh, g_sh, fg_xor_sh);
        interval_sh.Set(i, fg_xor_sh);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::vector<uint64_t> interval;
    brss_.Open(chls, interval_sh, interval);
    Logger::DebugLog(LOC, party_str + "Interval: " + ToString(interval));
#endif

    zt_eval_.Evaluate(chls, key.zt_keys, interval_sh, result);
}

void FssFMIEvaluator::EvaluateLPM_ShiftedAdditive(Channels                     &chls,
                                                  const FssFMIKey              &key,
                                                  std::vector<block>           &uv_prev,
                                                  std::vector<block>           &uv_next,
                                                  const sharing::RepShareMat64 &wm_tables,
                                                  const sharing::RepShareMat64 &query,
                                                  sharing::RepShareVec64       &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t qs       = params_.GetQuerySize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssFMI key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
    Logger::DebugLog(LOC, "Query size: " + ToString(qs));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    sharing::RepShare64    f_sh(0, 0), g_sh(0, 0);
    sharing::RepShare64    f_next_sh(0, 0), g_next_sh(0, 0);
    sharing::RepShareVec64 interval_sh(qs);

    if (party_id == 0) {
        g_sh.data[0] = wm_tables.RowView(0).Size() - 1;
    } else if (party_id == 1) {
        g_sh.data[1] = wm_tables.RowView(0).Size() - 1;
    }

    for (uint64_t i = 0; i < qs; ++i) {
        wm_eval_.EvaluateRankCF_ShiftedAdditive(chls, key.wm_f_keys[i], uv_prev, uv_next, wm_tables, query.RowView(i), f_sh, f_next_sh);
        wm_eval_.EvaluateRankCF_ShiftedAdditive(chls, key.wm_g_keys[i], uv_prev, uv_next, wm_tables, query.RowView(i), g_sh, g_next_sh);
        f_sh = f_next_sh;
        g_sh = g_next_sh;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint64_t f, g;
        brss_.Open(chls, f_sh, f);
        brss_.Open(chls, g_sh, g);
        Logger::InfoLog(LOC, party_str + "f(" + ToString(i) + "): " + ToString(f));
        Logger::InfoLog(LOC, party_str + "g(" + ToString(i) + "): " + ToString(g));
#endif
        sharing::RepShare64 fg_xor_sh;
        brss_.EvaluateXor(f_sh, g_sh, fg_xor_sh);
        interval_sh.Set(i, fg_xor_sh);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::vector<uint64_t> interval;
    brss_.Open(chls, interval_sh, interval);
    Logger::DebugLog(LOC, party_str + "Interval: " + ToString(interval));
#endif

    zt_eval_.Evaluate(chls, key.zt_keys, interval_sh, result);
}

void FssFMIEvaluator::EvaluateLPM_ShiftedAdditive_Parallel(Channels                     &chls,
                                                           const FssFMIKey              &key,
                                                           std::vector<block>           &uv_prev,
                                                           std::vector<block>           &uv_next,
                                                           const sharing::RepShareMat64 &wm_tables,
                                                           const sharing::RepShareMat64 &query,
                                                           sharing::RepShareVec64       &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t qs       = params_.GetQuerySize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssFMI key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
    Logger::DebugLog(LOC, "Query size: " + ToString(qs));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    sharing::RepShareVec64 fg_sh(2);
    sharing::RepShareVec64 fg_next_sh(2);
    sharing::RepShareVec64 interval_sh(qs);

    if (party_id == 0) {
        fg_sh.data[0][1] = wm_tables.RowView(0).Size() - 1;
    } else if (party_id == 1) {
        fg_sh.data[1][1] = wm_tables.RowView(0).Size() - 1;
    }

    for (uint64_t i = 0; i < qs; ++i) {
        wm_eval_.EvaluateRankCF_ShiftedAdditive_Parallel(chls, key.wm_f_keys[i], key.wm_g_keys[i],
                                                         uv_prev, uv_next, wm_tables,
                                                         query.RowView(i), fg_sh, fg_next_sh);
        fg_sh = fg_next_sh;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        std::vector<uint64_t> fg(2);
        brss_.Open(chls, fg_sh, fg);
        Logger::InfoLog(LOC, party_str + "f(" + ToString(i) + "): " + ToString(fg[0]));
        Logger::InfoLog(LOC, party_str + "g(" + ToString(i) + "): " + ToString(fg[1]));
#endif
        sharing::RepShare64 fg_xor_sh;
        brss_.EvaluateXor(fg_sh.At(0), fg_sh.At(1), fg_xor_sh);
        interval_sh.Set(i, fg_xor_sh);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::vector<uint64_t> interval;
    brss_.Open(chls, interval_sh, interval);
    Logger::DebugLog(LOC, party_str + "Interval: " + ToString(interval));
#endif

    zt_eval_.Evaluate(chls, key.zt_keys, interval_sh, result);
}

}    // namespace fm_index
}    // namespace fsswm
