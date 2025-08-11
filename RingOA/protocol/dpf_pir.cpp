#include "dpf_pir.h"

#include <cstring>

#include "RingOA/fss/prg.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace ringoa {
namespace proto {

void DpfPirParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Dpf Pir Parameters]" + GetParametersInfo());
}

DpfPirKey::DpfPirKey(const uint64_t id, const DpfPirParameters &params)
    : dpf_key(id, params.GetParameters()), r_sh(0), w_sh(0),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
}

size_t DpfPirKey::CalculateSerializedSize() const {
    return dpf_key.GetSerializedSize() + sizeof(r_sh) + sizeof(w_sh);
}

void DpfPirKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing DpfPirKey");
#endif

    // Serialize the DPF keys
    std::vector<uint8_t> key_buffer;
    dpf_key.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the random shares
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&r_sh), reinterpret_cast<const uint8_t *>(&r_sh) + sizeof(r_sh));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&w_sh), reinterpret_cast<const uint8_t *>(&w_sh) + sizeof(w_sh));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void DpfPirKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing DpfPirKey");
#endif
    size_t offset = 0;

    // Deserialize the DPF keys
    size_t key_size = dpf_key.GetSerializedSize();
    dpf_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the random shares
    std::memcpy(&r_sh, buffer.data() + offset, sizeof(r_sh));
    offset += sizeof(r_sh);
    std::memcpy(&w_sh, buffer.data() + offset, sizeof(w_sh));
}

void DpfPirKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("DpfPir Key"));
        dpf_key.PrintKey(true);
        Logger::DebugLog(LOC, "(r_sh=" + ToString(r_sh) + ", w_sh=" + ToString(w_sh) + ")");
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "DpfPir Key");
        dpf_key.PrintKey(false);
        Logger::DebugLog(LOC, "(r_sh=" + ToString(r_sh) + ", w_sh=" + ToString(w_sh) + ")");
    }
#endif
}

DpfPirKeyGenerator::DpfPirKeyGenerator(
    const DpfPirParameters     &params,
    sharing::AdditiveSharing2P &ss)
    : params_(params),
      gen_(params.GetParameters()), ss_(ss) {
}

void DpfPirKeyGenerator::OfflineSetUp(const uint64_t num_access, const std::string &file_path) const {
    ss_.OfflineSetUp(num_access, file_path + "bt");
}

std::pair<DpfPirKey, DpfPirKey> DpfPirKeyGenerator::GenerateKeys() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate DpfPir Keys"));
#endif

    // Initialize the keys
    std::array<DpfPirKey, 2>        keys     = {DpfPirKey(0, params_), DpfPirKey(1, params_)};
    std::pair<DpfPirKey, DpfPirKey> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    uint64_t d             = params_.GetDatabaseSize();
    uint64_t remaining_bit = params_.GetParameters().GetInputBitsize() - params_.GetParameters().GetTerminateBitsize();
    uint64_t w             = 0;
    block    final_seed_0, final_seed_1;
    bool     final_control_bit_1;

    // Generate random inputs for the keys
    uint64_t r = ss_.GenerateRandomValue();

    // Generate DPF keys
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> dpf_keys = gen_.GenerateKeys(r, 1, final_seed_0, final_seed_1, final_control_bit_1);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "r: " + ToString(r));
    Logger::DebugLog(LOC, "final_seed_0: " + Format(final_seed_0));
    Logger::DebugLog(LOC, "final_seed_1: " + Format(final_seed_1));
    Logger::DebugLog(LOC, "final_control_bit_1: " + ToString(final_control_bit_1));
#endif

    if (final_control_bit_1) {
        uint64_t fs0 = GetBit(final_seed_0, GetLowerNBits(r, remaining_bit));
        if (fs0) {
            w = 1;
        } else {
            w = Mod(-1, d);
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "fs0: " + ToString(fs0) + ", alpha_hat: " + ToString(GetLowerNBits(r, remaining_bit)));
        Logger::DebugLog(LOC, "w: " + ToString(w));
#endif
    } else {
        uint64_t fs1 = GetBit(final_seed_1, GetLowerNBits(r, remaining_bit));
        if (fs1) {
            w = Mod(-1, d);
        } else {
            w = 1;
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "fs1: " + ToString(fs1) + ", alpha_hat: " + ToString(GetLowerNBits(r, remaining_bit)));
        Logger::DebugLog(LOC, "w: " + ToString(w));
#endif
    }

    std::pair<uint64_t, uint64_t> r_sh = ss_.Share(r);
    std::pair<uint64_t, uint64_t> w_sh = ss_.Share(w);

    // Set DPF keys for each party
    key_pair.first.dpf_key  = std::move(dpf_keys.first);
    key_pair.second.dpf_key = std::move(dpf_keys.second);

    // Generate shared random values
    key_pair.first.r_sh  = r_sh.first;
    key_pair.second.r_sh = r_sh.second;
    key_pair.first.w_sh  = w_sh.first;
    key_pair.second.w_sh = w_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    key_pair.first.PrintKey();
    key_pair.second.PrintKey();
#endif

    // Return the generated keys as a pair.
    return key_pair;
}

DpfPirEvaluator::DpfPirEvaluator(
    const DpfPirParameters     &params,
    sharing::AdditiveSharing2P &ss)
    : params_(params), eval_(params.GetParameters()), ss_(ss),
      G_(fss::prg::PseudoRandomGenerator::GetInstance()) {
}

void DpfPirEvaluator::OnlineSetUp(const uint64_t party_id, const std::string &file_path) const {
    ss_.OnlineSetUp(party_id, file_path + "bt");
}

uint64_t DpfPirEvaluator::Evaluate(osuCrypto::Channel          &chl,
                                   const DpfPirKey             &key,
                                   std::vector<block>          &uv,
                                   const std::vector<uint64_t> &database,
                                   const uint64_t               index) const {

    if (params_.GetParameters().GetOutputBitsize() != 1) {
        Logger::ErrorLog(LOC, "Output bitsize must be 1, but got: " +
                                  ToString(params_.GetParameters().GetOutputBitsize()));
        return 0;
    }

    uint64_t party_id = key.dpf_key.party_id;
    uint64_t d        = params_.GetDatabaseSize();
    uint64_t nu       = params_.GetParameters().GetTerminateBitsize();

    if (uv.size() != (1UL << nu)) {
        Logger::ErrorLog(LOC, "Output vector size does not match the number of nodes: " +
                                  ToString(uv.size()) + " != " + ToString(1UL << nu));
    }
    if (database.size() != (1UL << d)) {
        Logger::ErrorLog(LOC, "Database size does not match the number of nodes: " +
                                  ToString(database.size()) + " != " + ToString(1UL << d));
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating DpfPirEvaluator protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " index: " + ToString(index));
#endif

    // Reconstruct masked inputs
    uint64_t pr_0, pr_1, pr;
    if (party_id == 0) {
        ss_.EvaluateSub(index, key.r_sh, pr_0);
        ss_.Reconst(party_id, chl, pr_0, pr_1, pr);
    } else {
        ss_.EvaluateSub(index, key.r_sh, pr_1);
        ss_.Reconst(party_id, chl, pr_0, pr_1, pr);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_sh: " + ToString(pr_0) + ", " + ToString(pr_1));
    Logger::DebugLog(LOC, party_str + " pr: " + ToString(pr));
#endif

    uint64_t dp = EvaluateFullDomainThenDotProduct(key.dpf_key, database, pr, uv);

    uint64_t dp_cor_sh;
    ss_.EvaluateMult(party_id, chl, dp, key.w_sh, dp_cor_sh);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " dp_cor_sh: " + ToString(dp_cor_sh));
#endif

    return dp_cor_sh;
}

uint64_t DpfPirEvaluator::EvaluateNaive(osuCrypto::Channel          &chl,
                                        const DpfPirKey             &key,
                                        std::vector<uint64_t>       &uv,
                                        const std::vector<uint64_t> &database,
                                        const uint64_t               index) const {

    if (params_.GetParameters().GetOutputBitsize() == 1) {
        Logger::ErrorLog(LOC, "Output bitsize must be larger than 1, but got: " +
                                  ToString(params_.GetParameters().GetOutputBitsize()));
        return 0;
    }

    uint64_t party_id = key.dpf_key.party_id;
    uint64_t d        = params_.GetDatabaseSize();

    if (uv.size() != (1UL << d)) {
        Logger::ErrorLog(LOC, "Output vector size does not match the number of nodes: " +
                                  ToString(uv.size()) + " != " + ToString(1UL << d));
    }
    if (database.size() != (1UL << d)) {
        Logger::ErrorLog(LOC, "Database size does not match the number of nodes: " +
                                  ToString(database.size()) + " != " + ToString(1UL << d));
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating DpfPirEvaluator protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " index: " + ToString(index));
#endif

    // Reconstruct masked inputs
    uint64_t pr_0, pr_1, pr;
    if (party_id == 0) {
        ss_.EvaluateSub(index, key.r_sh, pr_0);
        ss_.Reconst(party_id, chl, pr_0, pr_1, pr);
    } else {
        ss_.EvaluateSub(index, key.r_sh, pr_1);
        ss_.Reconst(party_id, chl, pr_0, pr_1, pr);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_sh: " + ToString(pr_0) + ", " + ToString(pr_1));
    Logger::DebugLog(LOC, party_str + " pr: " + ToString(pr));
#endif

    // Evaluate the FDE
    eval_.EvaluateFullDomain(key.dpf_key, uv);
    uint64_t db_sum = 0;

    for (size_t i = 0; i < uv.size(); ++i) {
        db_sum = Mod(db_sum + (database[Mod(i + pr, d)]) * uv[i], d);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " db_sum: " + ToString(db_sum));
#endif

    return db_sum;
}

uint64_t DpfPirEvaluator::EvaluateFullDomainThenDotProduct(const fss::dpf::DpfKey      &key,
                                                           const std::vector<uint64_t> &database,
                                                           const uint64_t               masked_idx,
                                                           std::vector<block>          &outputs) const {

    uint64_t party_id = key.party_id;
    uint64_t d        = params_.GetDatabaseSize();

    // Evaluate the FDE
    eval_.EvaluateFullDomain(key, outputs);
    uint64_t db_sum = 0;

    for (size_t i = 0; i < outputs.size(); ++i) {
        const uint64_t low  = outputs[i].get<uint64_t>()[0];
        const uint64_t high = outputs[i].get<uint64_t>()[1];

        for (int j = 0; j < 64; ++j) {
            const uint64_t mask = 0ULL - ((low >> j) & 1ULL);
            db_sum              = Mod(db_sum + Sign(party_id) * (database[Mod((i * 128 + j) + masked_idx, d)] & mask), d);
        }
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask = 0ULL - ((high >> j) & 1ULL);
            db_sum              = Mod(db_sum + Sign(party_id) * (database[Mod((i * 128 + 64 + j) + masked_idx, d)] & mask), d);
        }
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + ToString(db_sum));
#endif
    return db_sum;
}

}    // namespace proto
}    // namespace ringoa
