#include "oquantile_fsc.h"

#include <cstring>

#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"
#include "plain_wm.h"

namespace ringoa {
namespace wm {

using ringoa::sharing::ReplicatedSharing3P;

void OQuantileFscParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[OQuantileFsc Parameters]" + GetParametersInfo());
}

OQuantileFscKey::OQuantileFscKey(const uint64_t id, const OQuantileFscParameters &params)
    : num_oa_keys(params.GetSigma() * 2), num_ic_keys(params.GetSigma()),
      params_(params) {
    oa_keys.reserve(num_oa_keys);
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        oa_keys.emplace_back(proto::RingOaFscKey(id, params.GetOaParameters()));
    }
    ic_keys.reserve(num_ic_keys);
    for (uint64_t i = 0; i < num_ic_keys; ++i) {
        ic_keys.emplace_back(proto::IntegerComparisonKey(id, params.GetIcParameters()));
    }
    serialized_size_ = CalculateSerializedSize();
}

size_t OQuantileFscKey::CalculateSerializedSize() const {
    size_t size = sizeof(num_oa_keys);
    size += sizeof(num_ic_keys);
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        size += oa_keys[i].GetSerializedSize();
    }
    for (uint64_t i = 0; i < num_ic_keys; ++i) {
        size += ic_keys[i].GetSerializedSize();
    }
    return size;
}

void OQuantileFscKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing OQuantileFscKey");
#endif

    // Serialize the number of OA keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_oa_keys), reinterpret_cast<const uint8_t *>(&num_oa_keys) + sizeof(num_oa_keys));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_ic_keys), reinterpret_cast<const uint8_t *>(&num_ic_keys) + sizeof(num_ic_keys));

    // Serialize the OA keys
    for (const auto &oa_key : oa_keys) {
        std::vector<uint8_t> key_buffer;
        oa_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }
    // Serialize the IC keys
    for (const auto &ic_key : ic_keys) {
        std::vector<uint8_t> key_buffer;
        ic_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void OQuantileFscKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing OQuantileFscKey");
#endif
    size_t offset = 0;

    // Deserialize the number of OA keys
    std::memcpy(&num_oa_keys, buffer.data() + offset, sizeof(num_oa_keys));
    offset += sizeof(num_oa_keys);
    std::memcpy(&num_ic_keys, buffer.data() + offset, sizeof(num_ic_keys));
    offset += sizeof(num_ic_keys);

    // Deserialize the OA keys
    for (auto &oa_key : oa_keys) {
        size_t key_size = oa_key.GetSerializedSize();
        oa_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
    // Deserialize the IC keys
    for (auto &ic_key : ic_keys) {
        size_t key_size = ic_key.GetSerializedSize();
        ic_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
}

void OQuantileFscKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("OQuantileFsc Key"));
    Logger::DebugLog(LOC, "Number of RingOa Keys: " + ToString(num_oa_keys));
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        oa_keys[i].PrintKey(detailed);
    }
    Logger::DebugLog(LOC, "Number of IntegerComparison Keys: " + ToString(num_ic_keys));
    for (uint64_t i = 0; i < num_ic_keys; ++i) {
        ic_keys[i].PrintKey(detailed);
    }
}

OQuantileFscKeyGenerator::OQuantileFscKeyGenerator(
    const OQuantileFscParameters &params,
    sharing::AdditiveSharing2P   &ass,
    sharing::ReplicatedSharing3P &rss)
    : params_(params),
      oa_gen_(params.GetOaParameters(), rss, ass),
      ic_gen_(params.GetIcParameters(), ass, ass),
      rss_(rss) {
}

void OQuantileFscKeyGenerator::GenerateDatabaseU64Share(const WaveletMatrix                   &wm,
                                                        std::array<sharing::RepShareMat64, 3> &db_sh,
                                                        std::array<sharing::RepShareVec64, 3> &aux_sh,
                                                        std::array<bool, 3>                   &v_sign) const {
    if (wm.GetLength() + 1 != params_.GetDatabaseSize()) {
        throw std::invalid_argument("WaveletMatrix length does not match the database size in OQuantileFscParameters");
    }
    const std::vector<uint64_t> &rank0_tables = wm.GetRank0Tables();
    oa_gen_.GenerateDatabaseShare(rank0_tables, db_sh, wm.GetSigma(), wm.GetLength() + 1, v_sign);

    const size_t          stride = wm.GetLength() + 1;
    std::vector<uint64_t> total_zero(wm.GetSigma());
    for (size_t i = 0; i < wm.GetSigma(); ++i) {
        total_zero[i] = rank0_tables[(i + 1) * stride - 1];
    }
    aux_sh = rss_.ShareLocal(total_zero);
}

std::array<OQuantileFscKey, 3> OQuantileFscKeyGenerator::GenerateKeys(std::array<bool, 3> &v_sign) const {
    // Initialize the keys
    std::array<OQuantileFscKey, 3> keys = {
        OQuantileFscKey(0, params_),
        OQuantileFscKey(1, params_),
        OQuantileFscKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate OQuantileFsc keys"));
#endif

    for (uint64_t i = 0; i < keys[0].num_oa_keys; ++i) {
        // Generate the RingOa keys
        std::array<proto::RingOaFscKey, 3> oa_key = oa_gen_.GenerateKeys(v_sign);

        // Set the RingOa keys
        keys[0].oa_keys[i] = std::move(oa_key[0]);
        keys[1].oa_keys[i] = std::move(oa_key[1]);
        keys[2].oa_keys[i] = std::move(oa_key[2]);
    }

    for (uint64_t i = 0; i < keys[0].num_ic_keys; ++i) {
        // Generate the IntegerComparison keys
        std::pair<proto::IntegerComparisonKey, proto::IntegerComparisonKey> ic_key = ic_gen_.GenerateKeys();

        // Set the IntegerComparison keys
        keys[1].ic_keys[i] = std::move(ic_key.first);
        keys[2].ic_keys[i] = std::move(ic_key.second);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "OQuantileFsc keys generated");
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    // Return the keys
    return keys;
}

OQuantileFscEvaluator::OQuantileFscEvaluator(
    const OQuantileFscParameters &params,
    sharing::ReplicatedSharing3P &rss,
    sharing::AdditiveSharing2P   &ass_prev,
    sharing::AdditiveSharing2P   &ass_next)
    : params_(params),
      oa_eval_(params.GetOaParameters(), rss, ass_prev, ass_next),
      ic_eval_(params.GetIcParameters(), ass_prev, ass_next),
      rss_(rss) {
}

void OQuantileFscEvaluator::EvaluateQuantile(Channels                      &chls,
                                             const OQuantileFscKey         &key,
                                             std::vector<block>            &uv_prev,
                                             std::vector<block>            &uv_next,
                                             const sharing::RepShareMat64  &wm_tables,
                                             const sharing::RepShareView64 &aux_sh,
                                             sharing::RepShare64           &left_sh,
                                             sharing::RepShare64           &right_sh,
                                             sharing::RepShare64           &k_sh,
                                             sharing::RepShare64           &result) const {

    uint64_t s        = params_.GetShareSize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OQuantileFsc key"));
    Logger::DebugLog(LOC, "Share size: " + ToString(s));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    Logger::DebugLog(LOC, "Rows: " + ToString(wm_tables.rows) + ", Columns: " + ToString(wm_tables.cols));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    result = sharing::RepShare64(0, 0);
    sharing::RepShare64 zeroleft_sh(0, 0), zeroright_sh(0, 0);
    sharing::RepShare64 total_zeros(0, 0);
    sharing::RepShare64 zerocount_sh(0, 0);
    sharing::RepShare64 comp_sh(0, 0);

    size_t oa_key_idx = 0;
    for (uint64_t i = sigma; i > 0; --i) {
        const size_t bit = i - 1;
        oa_eval_.Evaluate(chls, key.oa_keys[oa_key_idx], uv_prev, uv_next, wm_tables.RowView(bit), left_sh, zeroleft_sh);
        oa_eval_.Evaluate(chls, key.oa_keys[oa_key_idx + 1], uv_prev, uv_next, wm_tables.RowView(bit), right_sh, zeroright_sh);
        oa_key_idx += 2;

        // total_zeros = wm_tables.RowView(bit).At(wm_tables.RowView(bit).Size() - 1);
        total_zeros = aux_sh.At(bit);
        rss_.EvaluateSub(zeroright_sh, zeroleft_sh, zerocount_sh);

        // Convert RSS to (2, 2)-sharing between P1 and P2 and Evaluate IntegerComparison
        uint64_t            ic_0{0}, ic_1{0};
        sharing::RepShare64 r1_sh, r2_sh;
        rss_.Rand(r1_sh);
        rss_.Rand(r2_sh);
        if (party_id == 1) {
            uint64_t k_0         = Mod2N(k_sh.data[0] + k_sh.data[1] + r1_sh.data[1], s);
            uint64_t zerocount_0 = Mod2N(zerocount_sh.data[0] + zerocount_sh.data[1] + r2_sh.data[1], s);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::InfoLog(LOC, party_str + " k_0: " + ToString(k_0) + ", zerocount_0: " + ToString(zerocount_0));
#endif
            ic_0 = ic_eval_.EvaluateSharedInput(chls.next, key.ic_keys[bit], k_0, zerocount_0);
        } else if (party_id == 2) {
            uint64_t k_1         = Mod2N(k_sh.data[0] - r1_sh.data[0], s);
            uint64_t zerocount_1 = Mod2N(zerocount_sh.data[0] - r2_sh.data[0], s);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::InfoLog(LOC, party_str + " k_1: " + ToString(k_1) + ", zerocount_1: " + ToString(zerocount_1));
#endif
            ic_1 = ic_eval_.EvaluateSharedInput(chls.prev, key.ic_keys[bit], k_1, zerocount_1);
        }

        // Convert (2, 2)-sharing to RSS
        if (party_id == 0) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(r1_sh[1] - r1_sh[0], s);
        } else if (party_id == 1) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(ic_0 + r1_sh[1] - r1_sh[0], s);
        } else if (party_id == 2) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(ic_1 + r1_sh[1] - r1_sh[0], s);
        }
        chls.next.send(comp_sh[0]);
        chls.prev.recv(comp_sh[1]);

        // Update k_sh
        sharing::RepShare64 update_sh(0, 0);
        rss_.EvaluateSub(k_sh, zerocount_sh, update_sh);
        rss_.EvaluateSelect(chls, k_sh, update_sh, comp_sh, k_sh);

        // Update left_sh and right_sh
        sharing::RepShare64 oneleft_sh(0, 0), oneright_sh(0, 0);
        rss_.EvaluateAdd(total_zeros, left_sh, oneleft_sh);
        rss_.EvaluateSub(oneleft_sh, zeroleft_sh, oneleft_sh);
        rss_.EvaluateAdd(total_zeros, right_sh, oneright_sh);
        rss_.EvaluateSub(oneright_sh, zeroright_sh, oneright_sh);
        rss_.EvaluateSelect(chls, zeroleft_sh, oneleft_sh, comp_sh, left_sh);
        rss_.EvaluateSelect(chls, zeroright_sh, oneright_sh, comp_sh, right_sh);

        // Update result
        sharing::RepShare64 cond_sh(0, 0);
        cond_sh[0] = Mod2N(comp_sh[0] * (1UL << bit), s);
        cond_sh[1] = Mod2N(comp_sh[1] * (1UL << bit), s);
        rss_.EvaluateAdd(result, cond_sh, result);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint64_t total_zero_rec, zeroleft_rec, zeroright_rec, zerocount_rec;
        uint64_t comp_rec;
        uint64_t k_rec;
        uint64_t left_rec, right_rec;
        uint64_t result_rec;
        rss_.Open(chls, total_zeros, total_zero_rec);
        rss_.Open(chls, zeroleft_sh, zeroleft_rec);
        rss_.Open(chls, zeroright_sh, zeroright_rec);
        rss_.Open(chls, zerocount_sh, zerocount_rec);
        rss_.Open(chls, comp_sh, comp_rec);
        rss_.Open(chls, k_sh, k_rec);
        rss_.Open(chls, left_sh, left_rec);
        rss_.Open(chls, right_sh, right_rec);
        rss_.Open(chls, result, result_rec);
        Logger::DebugLog(LOC, party_str + "total_zero_rec: " + ToString(total_zero_rec));
        Logger::DebugLog(LOC, party_str + "zeroleft_rec: " + ToString(zeroleft_rec));
        Logger::DebugLog(LOC, party_str + "zeroright_rec: " + ToString(zeroright_rec));
        Logger::DebugLog(LOC, party_str + "zerocount_rec: " + ToString(zerocount_rec));
        Logger::DebugLog(LOC, party_str + "comp_rec: " + ToString(comp_rec));
        Logger::DebugLog(LOC, party_str + "k_rec: " + ToString(k_rec));
        Logger::DebugLog(LOC, party_str + "left_rec: " + ToString(left_rec));
        Logger::DebugLog(LOC, party_str + "right_rec: " + ToString(right_rec));
        Logger::DebugLog(LOC, party_str + "result_rec: " + ToString(result_rec));
#endif
    }
}

void OQuantileFscEvaluator::EvaluateQuantile_Parallel(Channels                      &chls,
                                                      const OQuantileFscKey         &key,
                                                      std::vector<block>            &uv_prev,
                                                      std::vector<block>            &uv_next,
                                                      const sharing::RepShareMat64  &wm_tables,
                                                      const sharing::RepShareView64 &aux_sh,
                                                      sharing::RepShare64           &left_sh,
                                                      sharing::RepShare64           &right_sh,
                                                      sharing::RepShare64           &k_sh,
                                                      sharing::RepShare64           &result) const {

    uint64_t s        = params_.GetShareSize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OQuantileFsc_Parallel key"));
    Logger::DebugLog(LOC, "Share size: " + ToString(s));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    Logger::DebugLog(LOC, "Rows: " + ToString(wm_tables.rows) + ", Columns: " + ToString(wm_tables.cols));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    result = sharing::RepShare64(0, 0);
    sharing::RepShareVec64 lr_sh(2), zerolr_sh(2);
    sharing::RepShare64    total_zeros(0, 0);
    sharing::RepShare64    zerocount_sh(0, 0);
    sharing::RepShare64    comp_sh(0, 0);

    size_t oa_key_idx = 0;
    for (uint64_t i = sigma; i > 0; --i) {
        const size_t bit = i - 1;
        lr_sh.Set(0, left_sh);
        lr_sh.Set(1, right_sh);
        oa_eval_.Evaluate_Parallel(chls, key.oa_keys[oa_key_idx], key.oa_keys[oa_key_idx + 1], uv_prev, uv_next, wm_tables.RowView(bit), lr_sh, zerolr_sh);
        oa_key_idx += 2;

        // total_zeros = wm_tables.RowView(bit).At(wm_tables.RowView(bit).Size() - 1);
        total_zeros = aux_sh.At(bit);
        rss_.EvaluateSub(zerolr_sh.At(1), zerolr_sh.At(0), zerocount_sh);

        // Convert RSS to (2, 2)-sharing between P1 and P2 and Evaluate IntegerComparison
        uint64_t            ic_0{0}, ic_1{0};
        sharing::RepShare64 r1_sh, r2_sh;
        rss_.Rand(r1_sh);
        rss_.Rand(r2_sh);
        if (party_id == 1) {
            uint64_t k_0         = Mod2N(k_sh.data[0] + k_sh.data[1] + r1_sh.data[1], s);
            uint64_t zerocount_0 = Mod2N(zerocount_sh.data[0] + zerocount_sh.data[1] + r2_sh.data[1], s);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::InfoLog(LOC, party_str + " k_0: " + ToString(k_0) + ", zerocount_0: " + ToString(zerocount_0));
#endif
            ic_0 = ic_eval_.EvaluateSharedInput(chls.next, key.ic_keys[bit], k_0, zerocount_0);
        } else if (party_id == 2) {
            uint64_t k_1         = Mod2N(k_sh.data[0] - r1_sh.data[0], s);
            uint64_t zerocount_1 = Mod2N(zerocount_sh.data[0] - r2_sh.data[0], s);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::InfoLog(LOC, party_str + " k_1: " + ToString(k_1) + ", zerocount_1: " + ToString(zerocount_1));
#endif
            ic_1 = ic_eval_.EvaluateSharedInput(chls.prev, key.ic_keys[bit], k_1, zerocount_1);
        }

        // Convert (2, 2)-sharing to RSS
        if (party_id == 0) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(r1_sh[1] - r1_sh[0], s);
        } else if (party_id == 1) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(ic_0 + r1_sh[1] - r1_sh[0], s);
        } else if (party_id == 2) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(ic_1 + r1_sh[1] - r1_sh[0], s);
        }
        chls.next.send(comp_sh[0]);
        chls.prev.recv(comp_sh[1]);

        // Update k_sh
        sharing::RepShare64 update_sh(0, 0);
        rss_.EvaluateSub(k_sh, zerocount_sh, update_sh);
        rss_.EvaluateSelect(chls, k_sh, update_sh, comp_sh, k_sh);

        // Update left_sh and right_sh
        sharing::RepShare64 oneleft_sh(0, 0), oneright_sh(0, 0);
        rss_.EvaluateAdd(total_zeros, left_sh, oneleft_sh);
        rss_.EvaluateSub(oneleft_sh, zerolr_sh.At(0), oneleft_sh);
        rss_.EvaluateAdd(total_zeros, right_sh, oneright_sh);
        rss_.EvaluateSub(oneright_sh, zerolr_sh.At(1), oneright_sh);
        rss_.EvaluateSelect(chls, zerolr_sh.At(0), oneleft_sh, comp_sh, left_sh);
        rss_.EvaluateSelect(chls, zerolr_sh.At(1), oneright_sh, comp_sh, right_sh);

        // Update result
        sharing::RepShare64 cond_sh(0, 0);
        cond_sh[0] = Mod2N(comp_sh[0] * (1UL << bit), s);
        cond_sh[1] = Mod2N(comp_sh[1] * (1UL << bit), s);
        rss_.EvaluateAdd(result, cond_sh, result);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint64_t total_zero_rec, zeroleft_rec, zeroright_rec, zerocount_rec;
        uint64_t comp_rec;
        uint64_t k_rec;
        uint64_t left_rec, right_rec;
        uint64_t result_rec;
        rss_.Open(chls, total_zeros, total_zero_rec);
        rss_.Open(chls, zerolr_sh.At(0), zeroleft_rec);
        rss_.Open(chls, zerolr_sh.At(1), zeroright_rec);
        rss_.Open(chls, zerocount_sh, zerocount_rec);
        rss_.Open(chls, comp_sh, comp_rec);
        rss_.Open(chls, k_sh, k_rec);
        rss_.Open(chls, left_sh, left_rec);
        rss_.Open(chls, right_sh, right_rec);
        rss_.Open(chls, result, result_rec);
        Logger::DebugLog(LOC, party_str + "total_zero_rec: " + ToString(total_zero_rec));
        Logger::DebugLog(LOC, party_str + "zeroleft_rec: " + ToString(zeroleft_rec));
        Logger::DebugLog(LOC, party_str + "zeroright_rec: " + ToString(zeroright_rec));
        Logger::DebugLog(LOC, party_str + "zerocount_rec: " + ToString(zerocount_rec));
        Logger::DebugLog(LOC, party_str + "comp_rec: " + ToString(comp_rec));
        Logger::DebugLog(LOC, party_str + "k_rec: " + ToString(k_rec));
        Logger::DebugLog(LOC, party_str + "left_rec: " + ToString(left_rec));
        Logger::DebugLog(LOC, party_str + "right_rec: " + ToString(right_rec));
        Logger::DebugLog(LOC, party_str + "result_rec: " + ToString(result_rec));
#endif
    }
}

}    // namespace wm
}    // namespace ringoa
