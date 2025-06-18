#include "mixed_obliv_select.h"

#include <cstring>

#include "FssWM/fss/prg.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace proto {

void MixedOblivSelectParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Obliv Select Parameters]" + GetParametersInfo());
}

MixedOblivSelectKey::MixedOblivSelectKey(const uint64_t id, const MixedOblivSelectParameters &params)
    : party_id(id),
      key_from_prev(0, params.GetParameters()),
      key_from_next(1, params.GetParameters()),
      rsh_from_prev(0), rsh_from_next(0),
      wsh_from_prev(0), wsh_from_next(0),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
}

size_t MixedOblivSelectKey::CalculateSerializedSize() const {
    return sizeof(party_id) + key_from_prev.GetSerializedSize() + key_from_next.GetSerializedSize() +
           sizeof(rsh_from_prev) + sizeof(rsh_from_next) +
           sizeof(wsh_from_prev) + sizeof(wsh_from_next);
}

void MixedOblivSelectKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing MixedOblivSelectKey");
#endif
    // Serialize the party ID
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&party_id), reinterpret_cast<const uint8_t *>(&party_id) + sizeof(party_id));

    // Serialize the DPF keys
    std::vector<uint8_t> key_buffer;
    key_from_prev.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    key_buffer.clear();
    key_from_next.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the random shares
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&rsh_from_prev), reinterpret_cast<const uint8_t *>(&rsh_from_prev) + sizeof(rsh_from_prev));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&rsh_from_next), reinterpret_cast<const uint8_t *>(&rsh_from_next) + sizeof(rsh_from_next));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&wsh_from_prev), reinterpret_cast<const uint8_t *>(&wsh_from_prev) + sizeof(wsh_from_prev));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&wsh_from_next), reinterpret_cast<const uint8_t *>(&wsh_from_next) + sizeof(wsh_from_next));

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void MixedOblivSelectKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing MixedOblivSelectKey");
#endif
    size_t offset = 0;

    // Deserialize the party ID
    std::memcpy(&party_id, buffer.data() + offset, sizeof(party_id));
    offset += sizeof(party_id);

    // Deserialize the DPF keys
    size_t key_size = key_from_prev.GetSerializedSize();
    key_from_prev.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;
    key_size = key_from_next.GetSerializedSize();
    key_from_next.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the random shares
    std::memcpy(&rsh_from_prev, buffer.data() + offset, sizeof(rsh_from_prev));
    offset += sizeof(rsh_from_prev);
    std::memcpy(&rsh_from_next, buffer.data() + offset, sizeof(rsh_from_next));
    offset += sizeof(rsh_from_next);
    std::memcpy(&wsh_from_prev, buffer.data() + offset, sizeof(wsh_from_prev));
    offset += sizeof(wsh_from_prev);
    std::memcpy(&wsh_from_next, buffer.data() + offset, sizeof(wsh_from_next));
}

void MixedOblivSelectKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("MixedOblivSelect Key [Party " + ToString(party_id) + "]"));
        key_from_prev.PrintKey(true);
        key_from_next.PrintKey(true);
        Logger::DebugLog(LOC, "(rsh_from_prev, rsh_from_next): (" + ToString(rsh_from_prev) + ", " + ToString(rsh_from_next) + ")");
        Logger::DebugLog(LOC, "(wsh_from_prev, wsh_from_next): (" + ToString(wsh_from_prev) + ", " + ToString(wsh_from_next) + ")");
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "MixedOblivSelect Key [Party " + ToString(party_id) + "]");
        key_from_prev.PrintKey(false);
        key_from_next.PrintKey(false);
        Logger::DebugLog(LOC, "(rsh_from_prev, rsh_from_next): (" + ToString(rsh_from_prev) + ", " + ToString(rsh_from_next) + ")");
        Logger::DebugLog(LOC, "(wsh_from_prev, wsh_from_next): (" + ToString(wsh_from_prev) + ", " + ToString(wsh_from_next) + ")");
    }
#endif
}

void MixedOblivSelectKeyGenerator::OfflineSetUp(const uint64_t num_selection, const std::string &file_path) const {
    ass_.OfflineSetUp(num_selection, file_path + "btP0P1");
    ass_.OfflineSetUp(num_selection, file_path + "btP1P2");
    ass_.OfflineSetUp(num_selection, file_path + "btP2P0");
}

MixedOblivSelectKeyGenerator::MixedOblivSelectKeyGenerator(
    const MixedOblivSelectParameters &params,
    sharing::AdditiveSharing2P       &ass)
    : params_(params),
      gen_(params.GetParameters()), ass_(ass) {
}

std::array<MixedOblivSelectKey, 3> MixedOblivSelectKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<MixedOblivSelectKey, 3> keys = {
        MixedOblivSelectKey(0, params_),
        MixedOblivSelectKey(1, params_),
        MixedOblivSelectKey(2, params_),
    };
    uint64_t d             = params_.GetDatabaseSize();
    uint64_t remaining_bit = params_.GetParameters().GetInputBitsize() - params_.GetParameters().GetTerminateBitsize();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate MixedOblivSelect Keys"));
#endif

    std::array<uint64_t, 3>                                    rands;
    std::array<std::pair<uint64_t, uint64_t>, 3>               rand_shs;
    std::array<uint64_t, 3>                                    w;
    std::array<std::pair<uint64_t, uint64_t>, 3>               w_shs;
    std::vector<std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey>> key_pairs;
    key_pairs.reserve(3);

    block final_seed_0, final_seed_1;
    bool  final_control_bit_1;

    for (uint64_t i = 0; i < 3; ++i) {
        rands[i]    = ass_.GenerateRandomValue();
        rand_shs[i] = ass_.Share(rands[i]);
        key_pairs.push_back(gen_.GenerateKeys(rands[i], 1, final_seed_0, final_seed_1, final_control_bit_1));

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "final_seed_0: " + Format(final_seed_0));
        Logger::DebugLog(LOC, "final_seed_1: " + Format(final_seed_1));
        Logger::DebugLog(LOC, "final_control_bit_1: " + ToString(final_control_bit_1));
#endif

        if (final_control_bit_1) {
            uint64_t fs0 = GetBit(final_seed_0, GetLowerNBits(rands[i], remaining_bit));
            if (fs0) {
                w[i] = 1;
            } else {
                w[i] = Mod(-1, d);
            }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "fs0: " + ToString(fs0) + ", alpha_hat: " + ToString(GetLowerNBits(rands[i], remaining_bit)));
            Logger::DebugLog(LOC, "w[" + ToString(i) + "]: " + ToString(w[i]));
#endif
        } else {
            uint64_t fs1 = GetBit(final_seed_1, GetLowerNBits(rands[i], remaining_bit));
            if (fs1) {
                w[i] = Mod(-1, d);
            } else {
                w[i] = 1;
            }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "fs1: " + ToString(fs1) + ", alpha_hat: " + ToString(GetLowerNBits(rands[i], remaining_bit)));
            Logger::DebugLog(LOC, "w[" + ToString(i) + "]: " + ToString(w[i]));
#endif
        }
        w_shs[i] = ass_.Share(w[i]);
    }

    // Assign previous and next keys
    for (uint64_t i = 0; i < 3; ++i) {
        uint64_t prev         = (i + 2) % 3;
        uint64_t next         = (i + 1) % 3;
        keys[i].key_from_prev = std::move(key_pairs[prev].first);
        keys[i].rsh_from_prev = rand_shs[prev].first;
        keys[i].wsh_from_prev = w_shs[prev].first;
        keys[i].key_from_next = std::move(key_pairs[next].second);
        keys[i].rsh_from_next = rand_shs[next].second;
        keys[i].wsh_from_next = w_shs[next].second;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif
    return keys;
}

MixedOblivSelectEvaluator::MixedOblivSelectEvaluator(
    const MixedOblivSelectParameters &params,
    sharing::ReplicatedSharing3P     &rss,
    sharing::AdditiveSharing2P       &ass_prev,
    sharing::AdditiveSharing2P       &ass_next)
    : params_(params), eval_(params.GetParameters()), rss_(rss), ass_prev_(ass_prev), ass_next_(ass_next) {
}

void MixedOblivSelectEvaluator::OnlineSetUp(const uint64_t party_id, const std::string &file_path) const {
    if (party_id == 0) {
        ass_prev_.OnlineSetUp(1, file_path + "btP2P0");
        ass_next_.OnlineSetUp(0, file_path + "btP0P1");
    } else if (party_id == 1) {
        ass_prev_.OnlineSetUp(1, file_path + "btP0P1");
        ass_next_.OnlineSetUp(0, file_path + "btP1P2");
    } else {
        ass_prev_.OnlineSetUp(1, file_path + "btP1P2");
        ass_next_.OnlineSetUp(0, file_path + "btP2P0");
    }
}

void MixedOblivSelectEvaluator::Evaluate(Channels                      &chls,
                                         const MixedOblivSelectKey     &key,
                                         std::vector<block>            &uv_prev,
                                         std::vector<block>            &uv_next,
                                         const sharing::RepShareView64 &database,
                                         const sharing::RepShare64     &index,
                                         sharing::RepShare64           &result) const {

    uint64_t party_id = chls.party_id;
    uint64_t d        = params_.GetDatabaseSize();
    uint64_t nu       = params_.GetParameters().GetTerminateBitsize();

    if (uv_prev.size() != (1UL << nu) || uv_next.size() != (1UL << nu)) {
        Logger::ErrorLog(LOC, "Output vector size does not match the number of nodes: " +
                                  ToString(uv_prev.size()) + " != " + ToString(1UL << nu) +
                                  " or " + ToString(uv_next.size()) + " != " + ToString(1UL << nu));
    }
    if (database.Size() != (1UL << d)) {
        Logger::ErrorLog(LOC, "Database size does not match the number of nodes: " +
                                  ToString(database.Size()) + " != " + ToString(1UL << d));
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate MixedOblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
    Logger::DebugLog(LOC, party_str + " idx: " + index.ToString());
    Logger::DebugLog(LOC, party_str + " db: " + database.ToString());
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructMaskedValue(chls, key, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + ToString(pr_prev) + ", pr_next: " + ToString(pr_next));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block
    auto [dp_prev, dp_next] = EvaluateFullDomainThenDotProduct(
        party_id, key.key_from_prev, key.key_from_next, uv_prev, uv_next, database, pr_prev, pr_next);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + "dp_prev: " + ToString(dp_prev) + ", dp_next: " + ToString(dp_next));
#endif

    uint64_t ext_dp_prev, ext_dp_next;
    if (party_id == 0) {
        ass_prev_.EvaluateMult(1, chls.prev, dp_prev, key.wsh_from_next, ext_dp_prev);    // P0 <-> P2
        ass_next_.EvaluateMult(0, chls.next, dp_next, key.wsh_from_prev, ext_dp_next);    // P0 <-> P1
    } else if (party_id == 1) {
        ass_next_.EvaluateMult(0, chls.next, dp_next, key.wsh_from_prev, ext_dp_next);    // P1 <-> P2
        ass_prev_.EvaluateMult(1, chls.prev, dp_prev, key.wsh_from_next, ext_dp_prev);    // P1 <-> P0
    } else {
        ass_prev_.EvaluateMult(1, chls.prev, dp_prev, key.wsh_from_next, ext_dp_prev);    // P1 <-> P2
        ass_next_.EvaluateMult(0, chls.next, dp_next, key.wsh_from_prev, ext_dp_next);    // P0 <-> P2
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + "ext_dp_prev: " + ToString(ext_dp_prev) + ", ext_dp_next: " + ToString(ext_dp_next));
#endif

    uint64_t            selected_sh = Mod(ext_dp_prev + ext_dp_next, d);
    sharing::RepShare64 r_sh;
    rss_.Rand(r_sh);
    result[0] = Mod(selected_sh + r_sh[0] - r_sh[1], d);
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " result: " + ToString(result[0]) + ", " + ToString(result[1]));
#endif
}

void MixedOblivSelectEvaluator::Evaluate_Parallel(Channels                      &chls,
                                                  const MixedOblivSelectKey     &key1,
                                                  const MixedOblivSelectKey     &key2,
                                                  std::vector<block>            &uv_prev,
                                                  std::vector<block>            &uv_next,
                                                  const sharing::RepShareView64 &database,
                                                  const sharing::RepShareVec64  &index,
                                                  sharing::RepShareVec64        &result) const {

    uint64_t party_id = chls.party_id;
    uint64_t d        = params_.GetDatabaseSize();
    uint64_t nu       = params_.GetParameters().GetTerminateBitsize();

    if (uv_prev.size() != (1UL << nu) || uv_next.size() != (1UL << nu)) {
        Logger::ErrorLog(LOC, "Output vector size does not match the number of nodes: " +
                                  ToString(uv_prev.size()) + " != " + ToString(1UL << nu) +
                                  " or " + ToString(uv_next.size()) + " != " + ToString(1UL << nu));
    }
    if (database.Size() != (1UL << d)) {
        Logger::ErrorLog(LOC, "Database size does not match the number of nodes: " +
                                  ToString(database.Size()) + " != " + ToString(1UL << d));
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
    Logger::DebugLog(LOC, party_str + " idx: " + index.ToString());
    Logger::DebugLog(LOC, party_str + " db: " + database.ToString());
#endif

    // Reconstruct p ^ r_i
    // pr: [pr_prev1, pr_next1, pr_prev2, pr_next2]
    std::array<uint64_t, 4> pr = ReconstructMaskedValue(chls, key1, key2, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev1: " + ToString(pr[0]) + ", pr_next1: " + ToString(pr[1]) +
                              ", pr_prev2: " + ToString(pr[2]) + ", pr_next2: " + ToString(pr[3]));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block
    auto [dp_prev1, dp_next1] = EvaluateFullDomainThenDotProduct(
        party_id, key1.key_from_prev, key1.key_from_next, uv_prev, uv_next, database, pr[0], pr[1]);
    auto [dp_prev2, dp_next2] = EvaluateFullDomainThenDotProduct(
        party_id, key2.key_from_prev, key2.key_from_next, uv_prev, uv_next, database, pr[2], pr[3]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + "dp_prev1: " + ToString(dp_prev1) + ", dp_next1: " + ToString(dp_next1));
    Logger::DebugLog(LOC, party_str + "dp_prev2: " + ToString(dp_prev2) + ", dp_next2: " + ToString(dp_next2));
#endif

    std::array<uint64_t, 2> ext_dp_prev, ext_dp_next;
    if (party_id == 0) {
        ass_prev_.EvaluateMult(1, chls.prev, {dp_prev1, dp_prev2}, {key1.wsh_from_next, key2.wsh_from_next}, ext_dp_prev);    // P0 <-> P2
        ass_next_.EvaluateMult(0, chls.next, {dp_next1, dp_next2}, {key1.wsh_from_prev, key2.wsh_from_next}, ext_dp_next);    // P0 <-> P1

    } else if (party_id == 1) {
        ass_next_.EvaluateMult(0, chls.next, {dp_next1, dp_next2}, {key1.wsh_from_prev, key2.wsh_from_next}, ext_dp_next);    // P1 <-> P2
        ass_prev_.EvaluateMult(1, chls.prev, {dp_prev1, dp_prev2}, {key1.wsh_from_next, key2.wsh_from_next}, ext_dp_prev);    // P1 <-> P0
    } else {
        ass_prev_.EvaluateMult(1, chls.prev, {dp_prev1, dp_prev2}, {key1.wsh_from_next, key2.wsh_from_next}, ext_dp_prev);    // P1 <-> P2
        ass_next_.EvaluateMult(0, chls.next, {dp_next1, dp_next2}, {key1.wsh_from_prev, key2.wsh_from_next}, ext_dp_next);    // P0 <-> P2
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + "ext_dp_prev1: " + ToString(ext_dp_prev[0]) + ", ext_dp_prev2: " + ToString(ext_dp_prev[1]) +
                              ", ext_dp_next1: " + ToString(ext_dp_next[0]) + ", ext_dp_next2: " + ToString(ext_dp_next[1]));
#endif

    uint64_t            selected1_sh = Mod(dp_prev1 + dp_next1, d);
    uint64_t            selected2_sh = Mod(dp_prev2 + dp_next2, d);
    sharing::RepShare64 r1_sh, r2_sh;
    rss_.Rand(r1_sh);
    rss_.Rand(r2_sh);
    result[0][0] = Mod(selected1_sh + r1_sh[0] + r1_sh[1], d);
    result[0][1] = Mod(selected2_sh + r2_sh[0] + r2_sh[1], d);
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " result: " + ToString(result[0]) + ", " + ToString(result[1]));
#endif
}

std::pair<uint64_t, uint64_t> MixedOblivSelectEvaluator::EvaluateFullDomainThenDotProduct(
    const uint64_t                 party_id,
    const fss::dpf::DpfKey        &key_from_prev,
    const fss::dpf::DpfKey        &key_from_next,
    std::vector<block>            &uv_prev,
    std::vector<block>            &uv_next,
    const sharing::RepShareView64 &database,
    const uint64_t                 pr_prev,
    const uint64_t                 pr_next) const {

    uint64_t d = params_.GetDatabaseSize();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(party_id) + "] key_from_prev ID: " + ToString(key_from_prev.party_id));
    Logger::DebugLog(LOC, "[P" + ToString(party_id) + "] key_from_next ID: " + ToString(key_from_next.party_id));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block == __m128i)
    eval_.EvaluateFullDomain(key_from_next, uv_prev);
    eval_.EvaluateFullDomain(key_from_prev, uv_next);
    uint64_t dp_prev = 0, dp_next = 0;

    for (size_t i = 0; i < uv_prev.size(); ++i) {
        const uint64_t low_prev  = uv_prev[i].get<uint64_t>()[0];
        const uint64_t high_prev = uv_prev[i].get<uint64_t>()[1];
        const uint64_t low_next  = uv_next[i].get<uint64_t>()[0];
        const uint64_t high_next = uv_next[i].get<uint64_t>()[1];

        for (int j = 0; j < 64; ++j) {
            const uint64_t mask_prev = 0ULL - ((low_prev >> j) & 1ULL);
            const uint64_t mask_next = 0ULL - ((low_next >> j) & 1ULL);
            dp_prev                  = Mod(dp_prev + Sign(key_from_next.party_id) * (database.share1[Mod((i * 128 + j) + pr_prev, d)] & mask_prev), d);
            dp_next                  = Mod(dp_next + Sign(key_from_prev.party_id) * (database.share0[Mod((i * 128 + j) + pr_next, d)] & mask_next), d);
        }
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask_prev = 0ULL - ((high_prev >> j) & 1ULL);
            const uint64_t mask_next = 0ULL - ((high_next >> j) & 1ULL);
            dp_prev                  = Mod(dp_prev + Sign(key_from_next.party_id) * (database.share1[Mod((i * 128 + 64 + j) + pr_prev, d)] & mask_prev), d);
            dp_next                  = Mod(dp_next + Sign(key_from_prev.party_id) * (database.share0[Mod((i * 128 + 64 + j) + pr_next, d)] & mask_next), d);
        }
    }
    return std::make_pair(dp_prev, dp_next);
}

std::pair<uint64_t, uint64_t> MixedOblivSelectEvaluator::ReconstructMaskedValue(Channels                  &chls,
                                                                                const MixedOblivSelectKey &key,
                                                                                const sharing::RepShare64 &index) const {

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "ReconstructMaskedValue for Party " + ToString(chls.party_id));
#endif

    // Set replicated sharing of random value
    uint64_t d       = params_.GetDatabaseSize();
    uint64_t pr_prev = 0, pr_next = 0;

    // Reconstruct p - r_i
    if (chls.party_id == 0) {
        sharing::RepShare64 r_1_sh(key.rsh_from_next, 0);
        sharing::RepShare64 r_2_sh(0, key.rsh_from_prev);
        sharing::RepShare64 pr_20_sh, pr_01_sh;
        uint64_t            pr_20, pr_01;
        // p - r_1 between Party 2 (prev) and Party 0 (self)
        // p - r_2 between Party 0 (self) and Party 1 (next)
        rss_.EvaluateSub(index, r_1_sh, pr_20_sh);
        rss_.EvaluateSub(index, r_2_sh, pr_01_sh);
        chls.prev.send(pr_20_sh[0]);
        chls.next.send(pr_01_sh[1]);
        chls.next.recv(pr_01);
        chls.prev.recv(pr_20);
        pr_prev = Mod(pr_20 + pr_20_sh[0] + pr_20_sh[1], d);
        pr_next = Mod(pr_01_sh[0] + pr_01_sh[1] + pr_01, d);

    } else if (chls.party_id == 1) {
        sharing::RepShare64 r_0_sh(0, key.rsh_from_prev);
        sharing::RepShare64 r_2_sh(key.rsh_from_next, 0);
        sharing::RepShare64 pr_12_sh, pr_01_sh;
        uint64_t            pr_12, pr_01;
        // p - r_0 between Party 1 (self) and Party 2 (next)
        // p - r_2 between Party 0 (prev) and Party 1 (next)
        rss_.EvaluateSub(index, r_0_sh, pr_12_sh);
        rss_.EvaluateSub(index, r_2_sh, pr_01_sh);
        chls.next.send(pr_12_sh[1]);
        chls.prev.send(pr_01_sh[0]);
        chls.prev.recv(pr_01);
        chls.next.recv(pr_12);
        pr_prev = Mod(pr_01 + pr_01_sh[0] + pr_01_sh[1], d);
        pr_next = Mod(pr_12_sh[0] + pr_12_sh[1] + pr_12, d);

    } else {
        sharing::RepShare64 r_0_sh(key.rsh_from_next, 0);
        sharing::RepShare64 r_1_sh(0, key.rsh_from_prev);
        sharing::RepShare64 pr_12_sh, pr_20_sh;
        uint64_t            pr_12, pr_20;
        // p - r_0 between Party 1 (prev) and Party 2 (self)
        // p - r_1 between Party 2 (self) and Party 0 (next)
        rss_.EvaluateSub(index, r_0_sh, pr_12_sh);
        rss_.EvaluateSub(index, r_1_sh, pr_20_sh);
        chls.prev.send(pr_12_sh[0]);
        chls.next.send(pr_20_sh[1]);
        chls.prev.recv(pr_12);
        chls.next.recv(pr_20);
        pr_prev = Mod(pr_12 + pr_12_sh[0] + pr_12_sh[1], d);
        pr_next = Mod(pr_20_sh[0] + pr_20_sh[1] + pr_20, d);
    }
    return std::make_pair(pr_prev, pr_next);
}

std::array<uint64_t, 4> MixedOblivSelectEvaluator::ReconstructMaskedValue(Channels                     &chls,
                                                                          const MixedOblivSelectKey    &key1,
                                                                          const MixedOblivSelectKey    &key2,
                                                                          const sharing::RepShareVec64 &index) const {

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "ReconstructPR for Party " + ToString(chls.party_id));
#endif

    // Set replicated sharing of random value
    uint64_t               d        = params_.GetDatabaseSize();
    uint64_t               pr_prev1 = 0, pr_next1 = 0, pr_prev2 = 0, pr_next2 = 0;
    sharing::RepShareVec64 r_0_sh(2), r_1_sh(2), r_2_sh(2);

    // Reconstruct p - r_i
    if (chls.party_id == 0) {
        r_1_sh.Set(0, sharing::RepShare64(key1.rsh_from_next, 0));
        r_2_sh.Set(0, sharing::RepShare64(0, key1.rsh_from_prev));
        r_1_sh.Set(1, sharing::RepShare64(key2.rsh_from_next, 0));
        r_2_sh.Set(1, sharing::RepShare64(0, key2.rsh_from_prev));
        sharing::RepShareVec64 pr_20_sh(2), pr_01_sh(2);
        std::vector<uint64_t>  pr_20(2), pr_01(2);
        // p - r_1 between Party 0 and Party 2
        // p - r_2 between Party 0 and Party 1
        rss_.EvaluateSub(index, r_1_sh, pr_20_sh);
        rss_.EvaluateSub(index, r_2_sh, pr_01_sh);
        chls.prev.send(pr_20_sh[0]);
        chls.next.send(pr_01_sh[1]);
        chls.next.recv(pr_01);
        chls.prev.recv(pr_20);
        pr_prev1 = Mod(pr_20[0] + pr_20_sh[0][0] + pr_20_sh[1][0], d);
        pr_next1 = Mod(pr_01_sh[0][0] + pr_01_sh[1][0] + pr_01[0], d);
        pr_prev2 = Mod(pr_20[1] + pr_20_sh[0][1] + pr_20_sh[1][1], d);
        pr_next2 = Mod(pr_01_sh[0][1] + pr_01_sh[1][1] + pr_01[1], d);

    } else if (chls.party_id == 1) {
        r_0_sh.Set(0, sharing::RepShare64(0, key1.rsh_from_prev));
        r_2_sh.Set(0, sharing::RepShare64(key1.rsh_from_next, 0));
        r_0_sh.Set(1, sharing::RepShare64(0, key2.rsh_from_prev));
        r_2_sh.Set(1, sharing::RepShare64(key2.rsh_from_next, 0));
        sharing::RepShareVec64 pr_12_sh(2), pr_01_sh(2);
        std::vector<uint64_t>  pr_12(2), pr_01(2);
        // p - r_0 between Party 1 and Party 2
        // p - r_2 between Party 0 and Party 1
        rss_.EvaluateSub(index, r_0_sh, pr_12_sh);
        rss_.EvaluateSub(index, r_2_sh, pr_01_sh);
        chls.next.send(pr_12_sh[1]);
        chls.prev.send(pr_01_sh[0]);
        chls.prev.recv(pr_01);
        chls.next.recv(pr_12);
        pr_prev1 = Mod(pr_01[0] + pr_01_sh[0][0] + pr_01_sh[1][0], d);
        pr_next1 = Mod(pr_12_sh[0][0] + pr_12_sh[1][0] + pr_12[0], d);
        pr_prev2 = Mod(pr_01[1] + pr_01_sh[0][1] + pr_01_sh[1][1], d);
        pr_next2 = Mod(pr_12_sh[0][1] + pr_12_sh[1][1] + pr_12[1], d);

    } else {
        r_0_sh.Set(0, sharing::RepShare64(key1.rsh_from_next, 0));
        r_1_sh.Set(0, sharing::RepShare64(0, key1.rsh_from_prev));
        r_0_sh.Set(1, sharing::RepShare64(key2.rsh_from_next, 0));
        r_1_sh.Set(1, sharing::RepShare64(0, key2.rsh_from_prev));
        sharing::RepShareVec64 pr_12_sh(2), pr_20_sh(2);
        std::vector<uint64_t>  pr_12(2), pr_20(2);
        // p - r_0 between Party 1 and Party 2
        // p - r_1 between Party 0 and Party 2
        rss_.EvaluateSub(index, r_0_sh, pr_12_sh);
        rss_.EvaluateSub(index, r_1_sh, pr_20_sh);
        chls.prev.send(pr_12_sh[0]);
        chls.next.send(pr_20_sh[1]);
        chls.prev.recv(pr_12);
        chls.next.recv(pr_20);
        pr_prev1 = Mod(pr_12[0] + pr_12_sh[0][0] + pr_12_sh[1][0], d);
        pr_next1 = Mod(pr_20_sh[0][0] + pr_20_sh[1][0] + pr_20[0], d);
        pr_prev2 = Mod(pr_12[1] + pr_12_sh[0][1] + pr_12_sh[1][1], d);
        pr_next2 = Mod(pr_20_sh[0][1] + pr_20_sh[1][1] + pr_20[1], d);
    }
    return std::array<uint64_t, 4>{pr_prev1, pr_next1, pr_prev2, pr_next2};
}

}    // namespace proto
}    // namespace fsswm
