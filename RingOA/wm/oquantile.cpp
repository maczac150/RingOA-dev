#include "oquantile.h"

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

void OQuantileParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[OQuantile Parameters]" + GetParametersInfo());
}

OQuantileKey::OQuantileKey(const uint64_t id, const OQuantileParameters &params)
    : num_oa_keys(params.GetSigma() * 2), num_ic_keys(params.GetSigma()),
      params_(params) {
    oa_keys.reserve(num_oa_keys);
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        oa_keys.emplace_back(proto::RingOaKey(id, params.GetOaParameters()));
    }
    ic_keys.reserve(num_ic_keys);
    for (uint64_t i = 0; i < num_ic_keys; ++i) {
        ic_keys.emplace_back(proto::IntegerComparisonKey(id, params.GetIcParameters()));
    }
    serialized_size_ = CalculateSerializedSize();
}

size_t OQuantileKey::CalculateSerializedSize() const {
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

void OQuantileKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing OQuantileKey");
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

void OQuantileKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing OQuantileKey");
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

void OQuantileKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("OQuantile Key"));
    Logger::DebugLog(LOC, "Number of RingOa Keys: " + ToString(num_oa_keys));
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        oa_keys[i].PrintKey(detailed);
    }
    Logger::DebugLog(LOC, "Number of IntegerComparison Keys: " + ToString(num_ic_keys));
    for (uint64_t i = 0; i < num_ic_keys; ++i) {
        ic_keys[i].PrintKey(detailed);
    }
}

OQuantileKeyGenerator::OQuantileKeyGenerator(
    const OQuantileParameters    &params,
    sharing::AdditiveSharing2P   &ass,
    sharing::ReplicatedSharing3P &rss)
    : params_(params),
      oa_gen_(params.GetOaParameters(), ass),
      ic_gen_(params.GetIcParameters(), ass, ass),
      rss_(rss) {
}

void OQuantileKeyGenerator::OfflineSetUp(const std::string &file_path) {
    oa_gen_.OfflineSetUp(params_.GetSigma() * 2, file_path);
}

std::array<sharing::RepShareMat64, 3> OQuantileKeyGenerator::GenerateDatabaseU64Share(const WaveletMatrix &wm) const {
    if (wm.GetLength() + 1 != params_.GetDatabaseSize()) {
        throw std::invalid_argument("WaveletMatrix length does not match the database size in OQuantileParameters");
    }
    const std::vector<uint64_t> &rank0_tables = wm.GetRank0Tables();
    return rss_.ShareLocal(rank0_tables, wm.GetSigma(), wm.GetLength() + 1);
}

std::array<OQuantileKey, 3> OQuantileKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<OQuantileKey, 3> keys = {
        OQuantileKey(0, params_),
        OQuantileKey(1, params_),
        OQuantileKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate OQuantile keys"));
#endif

    for (uint64_t i = 0; i < keys[0].num_oa_keys; ++i) {
        // Generate the RingOa keys
        std::array<proto::RingOaKey, 3> oa_key = oa_gen_.GenerateKeys();

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
    Logger::DebugLog(LOC, "OQuantile keys generated");
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    // Return the keys
    return keys;
}

OQuantileEvaluator::OQuantileEvaluator(
    const OQuantileParameters    &params,
    sharing::ReplicatedSharing3P &rss,
    sharing::AdditiveSharing2P   &ass_prev,
    sharing::AdditiveSharing2P   &ass_next)
    : params_(params),
      oa_eval_(params.GetOaParameters(), rss, ass_prev, ass_next),
      ic_eval_(params.GetIcParameters(), ass_prev, ass_next),
      rss_(rss) {
}

void OQuantileEvaluator::OnlineSetUp(const uint64_t party_id, const std::string &file_path) {
    oa_eval_.OnlineSetUp(party_id, file_path);
}

void OQuantileEvaluator::EvaluateQuantile(Channels                     &chls,
                                          const OQuantileKey           &key,
                                          std::vector<block>           &uv_prev,
                                          std::vector<block>           &uv_next,
                                          const sharing::RepShareMat64 &wm_tables,
                                          sharing::RepShare64          &left_sh,
                                          sharing::RepShare64          &right_sh,
                                          sharing::RepShare64          &k_sh,
                                          sharing::RepShare64          &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OQuantile key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
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

        total_zeros = wm_tables.RowView(bit).At(wm_tables.RowView(bit).Size() - 1);
        rss_.EvaluateSub(zeroright_sh, zeroleft_sh, zerocount_sh);

        // Convert RSS to (2, 2)-sharing between P1 and P2 and Evaluate IntegerComparison
        uint64_t            ic_0, ic_1;
        sharing::RepShare64 r1_sh, r2_sh;
        rss_.Rand(r1_sh);
        rss_.Rand(r2_sh);
        if (party_id == 1) {
            uint64_t k_0         = Mod2N(k_sh.data[0] + k_sh.data[1] + r1_sh.data[1], d);
            uint64_t zerocount_0 = Mod2N(zerocount_sh.data[0] + zerocount_sh.data[1] + r2_sh.data[1], d);
            ic_0                 = ic_eval_.EvaluateSharedInput(chls.next, key.ic_keys[bit], k_0, zerocount_0);
        } else if (party_id == 2) {
            uint64_t k_1         = Mod2N(r1_sh.data[0] - r1_sh.data[0], d);
            uint64_t zerocount_1 = Mod2N(zerocount_sh.data[0] - r2_sh.data[0], d);
            ic_1                 = ic_eval_.EvaluateSharedInput(chls.prev, key.ic_keys[bit], k_1, zerocount_1);
        }

        // Convert (2, 2)-sharing to RSS
        if (party_id == 0) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(r1_sh[1] - r1_sh[0], d);
        } else if (party_id == 1) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(ic_0 + r1_sh[1] - r1_sh[0], d);
        } else if (party_id == 2) {
            rss_.Rand(r1_sh);
            comp_sh[0] = Mod2N(ic_1 + r1_sh[1] - r1_sh[0], d);
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
        cond_sh[0] = Mod2N(comp_sh[0] * (1UL << bit), d);
        cond_sh[1] = Mod2N(comp_sh[1] * (1UL << bit), d);
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

}    // namespace wm
}    // namespace ringoa
