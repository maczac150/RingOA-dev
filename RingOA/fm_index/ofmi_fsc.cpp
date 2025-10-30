#include "ofmi_fsc.h"

#include <cstring>

#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/binary_2p.h"
#include "RingOA/sharing/binary_3p.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"
#include "RingOA/wm/plain_wm.h"

namespace ringoa {
namespace fm_index {

void OFMIFscParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[OFMIFsc Parameters]" + GetParametersInfo());
}

OFMIFscKey::OFMIFscKey(const uint64_t id, const OFMIFscParameters &params)
    : num_wm_keys(params.GetQuerySize()),
      num_zt_keys(params.GetQuerySize()),
      params_(params) {
    wm_f_keys.reserve(num_wm_keys);
    wm_g_keys.reserve(num_wm_keys);
    zt_keys.reserve(num_zt_keys);
    for (uint64_t i = 0; i < num_wm_keys; ++i) {
        wm_f_keys.emplace_back(wm::OWMFscKey(id, params.GetOWMFscParameters()));
        wm_g_keys.emplace_back(wm::OWMFscKey(id, params.GetOWMFscParameters()));
    }
    for (uint64_t i = 0; i < num_zt_keys; ++i) {
        zt_keys.emplace_back(proto::ZeroTestKey(id, params.GetZeroTestParameters()));
    }
}

void OFMIFscKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing OFMIFscKey");
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

void OFMIFscKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing OFMIFscKey");
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

void OFMIFscKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("OFMIFsc Key"));
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

OFMIFscKeyGenerator::OFMIFscKeyGenerator(
    const OFMIFscParameters      &params,
    sharing::AdditiveSharing2P   &ass,
    sharing::ReplicatedSharing3P &rss)
    : params_(params),
      wm_gen_(params.GetOWMFscParameters(), ass, rss),
      zt_gen_(params.GetZeroTestParameters(), ass, ass),
      rss_(rss) {
}

void OFMIFscKeyGenerator::GenerateDatabaseU64Share(
    const wm::FMIndex                     &fm,
    std::array<sharing::RepShareMat64, 3> &db_sh,
    std::array<sharing::RepShareVec64, 3> &aux_sh,
    std::array<bool, 3>                   &v_sign) const {
    wm_gen_.GenerateDatabaseU64Share(fm, db_sh, aux_sh, v_sign);
}

std::array<sharing::RepShareMat64, 3> OFMIFscKeyGenerator::GenerateQueryU64Share(const wm::FMIndex &fm, std::string &query) const {
    std::vector<uint64_t> query_bv = fm.ConvertToBitMatrix(query);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query bitvec: " + ToStringMatrix(query_bv, params_.GetQuerySize(), fm.GetWaveletMatrix().GetSigma()));
#endif
    return rss_.ShareLocal(query_bv, params_.GetQuerySize(), fm.GetWaveletMatrix().GetSigma());
}

std::array<OFMIFscKey, 3> OFMIFscKeyGenerator::GenerateKeys(std::array<bool, 3> &v_sign) const {
    // Initialize keys
    std::array<OFMIFscKey, 3> keys = {
        OFMIFscKey(0, params_),
        OFMIFscKey(1, params_),
        OFMIFscKey(2, params_)};

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate OWMFsc keys"));
#endif

    for (uint64_t i = 0; i < keys[0].num_wm_keys; ++i) {
        // Generate the OWMFscKey keys
        std::array<wm::OWMFscKey, 3> wm_f_key = wm_gen_.GenerateKeys(v_sign);
        std::array<wm::OWMFscKey, 3> wm_g_key = wm_gen_.GenerateKeys(v_sign);

        // Set the OWMFscKey keys
        keys[0].wm_f_keys[i] = std::move(wm_f_key[0]);
        keys[1].wm_f_keys[i] = std::move(wm_f_key[1]);
        keys[2].wm_f_keys[i] = std::move(wm_f_key[2]);
        keys[0].wm_g_keys[i] = std::move(wm_g_key[0]);
        keys[1].wm_g_keys[i] = std::move(wm_g_key[1]);
        keys[2].wm_g_keys[i] = std::move(wm_g_key[2]);
    }

    for (uint64_t i = 0; i < keys[0].num_zt_keys; ++i) {
        // Generate the ZeroTestKey
        std::pair<proto::ZeroTestKey, proto::ZeroTestKey> zt_key = zt_gen_.GenerateKeys();

        // Set the ZeroTestKey keys
        keys[1].zt_keys[i] = std::move(zt_key.first);
        keys[2].zt_keys[i] = std::move(zt_key.second);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "OWMFsc keys generated");
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    // Return the keys
    return keys;
}

OFMIFscEvaluator::OFMIFscEvaluator(const OFMIFscParameters      &params,
                                   sharing::ReplicatedSharing3P &rss,
                                   sharing::AdditiveSharing2P   &ass_prev,
                                   sharing::AdditiveSharing2P   &ass_next)
    : params_(params),
      wm_eval_(params.GetOWMFscParameters(), rss, ass_prev, ass_next),
      zt_eval_(params.GetZeroTestParameters(), ass_prev, ass_next),
      rss_(rss), ass_prev_(ass_prev), ass_next_(ass_next) {
}

void OFMIFscEvaluator::EvaluateLPM(Channels                      &chls,
                                   const OFMIFscKey              &key,
                                   std::vector<block>            &uv_prev,
                                   std::vector<block>            &uv_next,
                                   const sharing::RepShareMat64  &wm_tables,
                                   const sharing::RepShareView64 &aux_sh,
                                   const sharing::RepShareMat64  &query,
                                   sharing::RepShareVec64        &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t qs       = params_.GetQuerySize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OFMIFsc key"));
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
        wm_eval_.EvaluateRankCF(chls, key.wm_f_keys[i], uv_prev, uv_next, wm_tables,
                                aux_sh, query.RowView(i), f_sh, f_next_sh);
        wm_eval_.EvaluateRankCF(chls, key.wm_g_keys[i], uv_prev, uv_next, wm_tables,
                                aux_sh, query.RowView(i), g_sh, g_next_sh);
        f_sh = f_next_sh;
        g_sh = g_next_sh;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint64_t f, g;
        rss_.Open(chls, f_sh, f);
        rss_.Open(chls, g_sh, g);
        Logger::InfoLog(LOC, party_str + "f(" + ToString(i) + "): " + ToString(f));
        Logger::InfoLog(LOC, party_str + "g(" + ToString(i) + "): " + ToString(g));
#endif
        sharing::RepShare64 fg_sub_sh;
        rss_.EvaluateSub(g_sh, f_sh, fg_sub_sh);
        interval_sh.Set(i, fg_sub_sh);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::vector<uint64_t> interval;
    rss_.Open(chls, interval_sh, interval);
    Logger::DebugLog(LOC, party_str + "Interval: " + ToString(interval));
#endif

    // Convert RSS to (2, 2)-sharing between P1 and P2 and Evaluate ZeroTest
    std::vector<uint64_t> masked_intervals_0(qs), masked_intervals_1(qs), masked_intervals(qs);
    std::vector<uint64_t> zt_0(qs), zt_1(qs), recon_zt(qs);
    sharing::RepShare64   r_sh;
    rss_.Rand(r_sh);
    if (party_id == 1) {
        for (uint64_t i = 0; i < qs; ++i) {
            uint64_t interval_0 = Mod2N(interval_sh.data[0][i] + interval_sh.data[1][i] + r_sh.data[1], d);
            uint64_t masked_interval_0;
            ass_next_.EvaluateAdd(interval_0, key.zt_keys[i].shr_in, masked_interval_0);
            masked_intervals_0[i] = masked_interval_0;
        }
        ass_next_.Reconst(0, chls.next, masked_intervals_0, masked_intervals_1, masked_intervals);
        for (uint64_t i = 0; i < qs; ++i) {
            zt_0[i] = zt_eval_.EvaluateMaskedInput(key.zt_keys[i], masked_intervals[i]);
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        ass_next_.Reconst(0, chls.next, zt_0, zt_1, recon_zt);
        Logger::DebugLog(LOC, party_str + "Reconstructed ZT: " + ToString(recon_zt));
#endif
    } else if (party_id == 2) {
        for (uint64_t i = 0; i < qs; ++i) {
            uint64_t interval_1 = Mod2N(interval_sh.data[0][i] - r_sh.data[0], d);
            uint64_t masked_interval_1;
            ass_prev_.EvaluateAdd(interval_1, key.zt_keys[i].shr_in, masked_interval_1);
            masked_intervals_1[i] = masked_interval_1;
        }
        ass_prev_.Reconst(1, chls.prev, masked_intervals_0, masked_intervals_1, masked_intervals);
        for (uint64_t i = 0; i < qs; ++i) {
            zt_1[i] = zt_eval_.EvaluateMaskedInput(key.zt_keys[i], masked_intervals[i]);
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        ass_prev_.Reconst(1, chls.prev, zt_0, zt_1, recon_zt);
        Logger::DebugLog(LOC, party_str + "Reconstructed ZT: " + ToString(recon_zt));
#endif
    }

    // Convert (2, 2)-sharing to RSS
    if (party_id == 0) {
        for (uint64_t i = 0; i < qs; ++i) {
            rss_.Rand(r_sh);
            result[0][i] = Mod2N(r_sh.data[1] - r_sh.data[0], d);
        }
    } else if (party_id == 1) {
        for (uint64_t i = 0; i < qs; ++i) {
            rss_.Rand(r_sh);
            result[0][i] = Mod2N(zt_0[i] + r_sh.data[1] - r_sh.data[0], d);
        }
    } else {
        for (uint64_t i = 0; i < qs; ++i) {
            rss_.Rand(r_sh);
            result[0][i] = Mod2N(zt_1[i] + r_sh.data[1] - r_sh.data[0], d);
        }
    }
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
}

void OFMIFscEvaluator::EvaluateLPM_Parallel(Channels                      &chls,
                                            const OFMIFscKey              &key,
                                            std::vector<block>            &uv_prev,
                                            std::vector<block>            &uv_next,
                                            const sharing::RepShareMat64  &wm_tables,
                                            const sharing::RepShareView64 &aux_sh,
                                            const sharing::RepShareMat64  &query,
                                            sharing::RepShareVec64        &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t qs       = params_.GetQuerySize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OFMIFsc key"));
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
        wm_eval_.EvaluateRankCF_Parallel(chls, key.wm_f_keys[i], key.wm_g_keys[i],
                                         uv_prev, uv_next, wm_tables, aux_sh,
                                         query.RowView(i), fg_sh, fg_next_sh);
        fg_sh = fg_next_sh;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        std::vector<uint64_t> fg(2);
        rss_.Open(chls, fg_sh, fg);
        Logger::InfoLog(LOC, party_str + "f(" + ToString(i) + "): " + ToString(fg[0]));
        Logger::InfoLog(LOC, party_str + "g(" + ToString(i) + "): " + ToString(fg[1]));
#endif
        sharing::RepShare64 fg_sub_sh;
        rss_.EvaluateSub(fg_sh.At(1), fg_sh.At(0), fg_sub_sh);
        interval_sh.Set(i, fg_sub_sh);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    std::vector<uint64_t> interval;
    rss_.Open(chls, interval_sh, interval);
    Logger::DebugLog(LOC, party_str + "Interval: " + ToString(interval));
#endif

    // Convert RSS to (2, 2)-sharing between P1 and P2 and Evaluate ZeroTest
    std::vector<uint64_t> masked_intervals_0(qs), masked_intervals_1(qs), masked_intervals(qs);
    std::vector<uint64_t> zt_0(qs), zt_1(qs), recon_zt(qs);
    sharing::RepShare64   r_sh;
    rss_.Rand(r_sh);
    if (party_id == 1) {
        for (uint64_t i = 0; i < qs; ++i) {
            uint64_t interval_0 = Mod2N(interval_sh.data[0][i] + interval_sh.data[1][i] + r_sh.data[1], d);
            uint64_t masked_interval_0;
            ass_next_.EvaluateAdd(interval_0, key.zt_keys[i].shr_in, masked_interval_0);
            masked_intervals_0[i] = masked_interval_0;
        }
        ass_next_.Reconst(0, chls.next, masked_intervals_0, masked_intervals_1, masked_intervals);
        for (uint64_t i = 0; i < qs; ++i) {
            zt_0[i] = zt_eval_.EvaluateMaskedInput(key.zt_keys[i], masked_intervals[i]);
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        ass_next_.Reconst(0, chls.next, zt_0, zt_1, recon_zt);
        Logger::DebugLog(LOC, party_str + "Reconstructed ZT: " + ToString(recon_zt));
#endif
    } else if (party_id == 2) {
        for (uint64_t i = 0; i < qs; ++i) {
            uint64_t interval_1 = Mod2N(interval_sh.data[0][i] - r_sh.data[0], d);
            uint64_t masked_interval_1;
            ass_prev_.EvaluateAdd(interval_1, key.zt_keys[i].shr_in, masked_interval_1);
            masked_intervals_1[i] = masked_interval_1;
        }
        ass_prev_.Reconst(1, chls.prev, masked_intervals_0, masked_intervals_1, masked_intervals);
        for (uint64_t i = 0; i < qs; ++i) {
            zt_1[i] = zt_eval_.EvaluateMaskedInput(key.zt_keys[i], masked_intervals[i]);
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        ass_prev_.Reconst(1, chls.prev, zt_0, zt_1, recon_zt);
        Logger::DebugLog(LOC, party_str + "Reconstructed ZT: " + ToString(recon_zt));
#endif
    }

    // Convert (2, 2)-sharing to RSS
    if (party_id == 0) {
        for (uint64_t i = 0; i < qs; ++i) {
            rss_.Rand(r_sh);
            result[0][i] = Mod2N(r_sh.data[1] - r_sh.data[0], d);
        }
    } else if (party_id == 1) {
        for (uint64_t i = 0; i < qs; ++i) {
            rss_.Rand(r_sh);
            result[0][i] = Mod2N(zt_0[i] + r_sh.data[1] - r_sh.data[0], d);
        }
    } else {
        for (uint64_t i = 0; i < qs; ++i) {
            rss_.Rand(r_sh);
            result[0][i] = Mod2N(zt_1[i] + r_sh.data[1] - r_sh.data[0], d);
        }
    }
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
}

}    // namespace fm_index
}    // namespace ringoa
