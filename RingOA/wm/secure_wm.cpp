#include "secure_wm.h"

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

void SecureWMParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[SecureWM Parameters]" + GetParametersInfo());
}

SecureWMKey::SecureWMKey(const uint64_t id, const SecureWMParameters &params)
    : num_oa_keys(params.GetSigma()),
      params_(params) {
    oa_keys.reserve(num_oa_keys);
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        oa_keys.emplace_back(proto::RingOaKey(id, params.GetOaParameters()));
    }
    serialized_size_ = CalculateSerializedSize();
}

size_t SecureWMKey::CalculateSerializedSize() const {
    size_t size = sizeof(num_oa_keys);
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        size += oa_keys[i].GetSerializedSize();
    }
    return size;
}

void SecureWMKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing SecureWMKey");
#endif

    // Serialize the number of OA keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_oa_keys), reinterpret_cast<const uint8_t *>(&num_oa_keys) + sizeof(num_oa_keys));

    // Serialize the OA keys
    for (const auto &oa_key : oa_keys) {
        std::vector<uint8_t> key_buffer;
        oa_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void SecureWMKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing SecureWMKey");
#endif
    size_t offset = 0;

    // Deserialize the number of OA keys
    std::memcpy(&num_oa_keys, buffer.data() + offset, sizeof(num_oa_keys));
    offset += sizeof(num_oa_keys);

    // Deserialize the OA keys
    for (auto &oa_key : oa_keys) {
        size_t key_size = oa_key.GetSerializedSize();
        oa_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
}

void SecureWMKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("SecureWM Key"));
    Logger::DebugLog(LOC, "Number of RingOa Keys: " + ToString(num_oa_keys));
    for (uint64_t i = 0; i < num_oa_keys; ++i) {
        oa_keys[i].PrintKey(detailed);
    }
}

SecureWMKeyGenerator::SecureWMKeyGenerator(
    const SecureWMParameters     &params,
    sharing::AdditiveSharing2P   &ass,
    sharing::ReplicatedSharing3P &rss)
    : params_(params),
      oa_gen_(params.GetOaParameters(), ass),
      rss_(rss) {
}

std::array<sharing::RepShareMat64, 3> SecureWMKeyGenerator::GenerateDatabaseU64Share(const FMIndex &fm) const {
    if (fm.GetWaveletMatrix().GetLength() + 1 != params_.GetDatabaseSize()) {
        throw std::invalid_argument("FMIndex length does not match the database size in SecureWMParameters");
    }
    const std::vector<uint64_t> &rank0_tables = fm.GetRank0Tables();
    return rss_.ShareLocal(rank0_tables, fm.GetWaveletMatrix().GetSigma(), fm.GetWaveletMatrix().GetLength() + 1);
}

std::array<SecureWMKey, 3> SecureWMKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<SecureWMKey, 3> keys = {
        SecureWMKey(0, params_),
        SecureWMKey(1, params_),
        SecureWMKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate SecureWM keys"));
#endif

    for (uint64_t i = 0; i < keys[0].num_oa_keys; ++i) {
        // Generate the RingOa keys
        std::array<proto::RingOaKey, 3> oa_key = oa_gen_.GenerateKeys();

        // Set the RingOa keys
        keys[0].oa_keys[i] = std::move(oa_key[0]);
        keys[1].oa_keys[i] = std::move(oa_key[1]);
        keys[2].oa_keys[i] = std::move(oa_key[2]);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "SecureWM keys generated");
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    // Return the keys
    return keys;
}

SecureWMEvaluator::SecureWMEvaluator(
    const SecureWMParameters     &params,
    sharing::ReplicatedSharing3P &rss,
    sharing::AdditiveSharing2P   &ass_prev,
    sharing::AdditiveSharing2P   &ass_next)
    : params_(params),
      oa_eval_(params.GetOaParameters(), rss, ass_prev, ass_next),
      rss_(rss) {
}

void SecureWMEvaluator::EvaluateRankCF(Channels                      &chls,
                                       const SecureWMKey             &key,
                                       std::vector<block>            &uv_prev,
                                       std::vector<block>            &uv_next,
                                       const sharing::RepShareMat64  &wm_tables,
                                       const sharing::RepShareView64 &char_sh,
                                       sharing::RepShare64           &position_sh,
                                       sharing::RepShare64           &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate SecureWM key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    Logger::DebugLog(LOC, "Rows: " + ToString(wm_tables.rows) + ", Columns: " + ToString(wm_tables.cols));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    sharing::RepShare64 rank0_sh(0, 0), rank1_sh(0, 0);
    sharing::RepShare64 total_zeros;
    sharing::RepShare64 p_sub_rank0_sh;

    for (uint64_t i = 0; i < sigma; ++i) {
        oa_eval_.Evaluate(chls, key.oa_keys[i], uv_prev, uv_next, wm_tables.RowView(i), position_sh, rank0_sh);

        total_zeros = wm_tables.RowView(i).At(wm_tables.RowView(i).Size() - 1);
        rss_.EvaluateSub(position_sh, rank0_sh, p_sub_rank0_sh);
        rss_.EvaluateAdd(p_sub_rank0_sh, total_zeros, rank1_sh);
        rss_.EvaluateSelect(chls, rank0_sh, rank1_sh, char_sh.At(i), position_sh);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        uint64_t total_zero_rec;
        uint64_t p_sub_rank0;
        rss_.Open(chls, total_zeros, total_zero_rec);
        rss_.Open(chls, p_sub_rank0_sh, p_sub_rank0);
        Logger::DebugLog(LOC, party_str + "total_zero_rec: " + ToString(total_zero_rec));
        Logger::DebugLog(LOC, party_str + "p_sub_rank0: " + ToString(p_sub_rank0));
        Logger::DebugLog(LOC, party_str + "Rank0 share: " + ToString(rank0_sh[0]) + ", " + ToString(rank0_sh[1]));
        Logger::DebugLog(LOC, party_str + "Rank1 share: " + ToString(rank1_sh[0]) + ", " + ToString(rank1_sh[1]));
        uint64_t open_position = 0;
        rss_.Open(chls, position_sh, open_position);
        Logger::DebugLog(LOC, party_str + "Rank CF for character " + ToString(i) + ": " + ToString(open_position));
#endif
    }
    result = position_sh;
}

void SecureWMEvaluator::EvaluateRankCF_Parallel(Channels                      &chls,
                                                const SecureWMKey             &key1,
                                                const SecureWMKey             &key2,
                                                std::vector<block>            &uv_prev,
                                                std::vector<block>            &uv_next,
                                                const sharing::RepShareMat64  &wm_tables,
                                                const sharing::RepShareView64 &char_sh,
                                                sharing::RepShareVec64        &position_sh,
                                                sharing::RepShareVec64        &result) const {
    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate SecureWM key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    Logger::DebugLog(LOC, "Rows: " + ToString(wm_tables.rows) + ", Columns: " + ToString(wm_tables.cols));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    sharing::RepShareVec64 rank0_sh(2), rank1_sh(2);
    sharing::RepShareVec64 total_zeros(2);
    sharing::RepShareVec64 p_sub_rank0_sh(2);
    for (uint64_t i = 0; i < sigma; ++i) {
        oa_eval_.Evaluate_Parallel(chls, key1.oa_keys[i], key2.oa_keys[i], uv_prev, uv_next, wm_tables.RowView(i), position_sh, rank0_sh);
        total_zeros.Set(0, wm_tables.RowView(i).At(wm_tables.RowView(i).Size() - 1));
        total_zeros.Set(1, wm_tables.RowView(i).At(wm_tables.RowView(i).Size() - 1));
        rss_.EvaluateSub(position_sh, rank0_sh, p_sub_rank0_sh);
        rss_.EvaluateAdd(p_sub_rank0_sh, total_zeros, rank1_sh);
        rss_.EvaluateSelect(chls, rank0_sh, rank1_sh, char_sh.At(i), position_sh);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + "Rank0 share: " + ToString(rank0_sh[0]) + ", " + ToString(rank0_sh[1]));
        Logger::DebugLog(LOC, party_str + "Rank1 share: " + ToString(rank1_sh[0]) + ", " + ToString(rank1_sh[1]));
        std::vector<uint64_t> open_position(2);
        rss_.Open(chls, position_sh, open_position);
        Logger::DebugLog(LOC, party_str + "Rank CF for character " + ToString(i) + ": " + ToString(open_position[0]) + ", " + ToString(open_position[1]));
#endif
    }
    result = position_sh;
}

}    // namespace wm
}    // namespace ringoa
