#include "obliv_select.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace wm {

void OblivSelectParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Obliv Select Parameters]" + GetParametersInfo());
}

OblivSelectKey::OblivSelectKey(const uint32_t id, const OblivSelectParameters &params)
    : party_id(id),
      prev_key(0, params.GetParameters()),
      next_key(1, params.GetParameters()),
      prev_r_sh(0), next_r_sh(0),
      r(0), r_sh_0(0), r_sh_1(0),
      params_(params),
      serialized_size_(GetSerializedSize()) {
}

size_t OblivSelectKey::CalculateSerializedSize() const {
    return sizeof(party_id) + prev_key.GetSerializedSize() + next_key.GetSerializedSize() +
           sizeof(prev_r_sh) + sizeof(next_r_sh) +
           sizeof(r) + sizeof(r_sh_0) + sizeof(r_sh_1);
}

void OblivSelectKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing OblivSelectKey");
#endif
    // Serialize the party ID
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&party_id), reinterpret_cast<const uint8_t *>(&party_id) + sizeof(party_id));

    // Serialize the DPF keys
    std::vector<uint8_t> key_buffer;
    prev_key.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    key_buffer.clear();
    next_key.Serialize(key_buffer);
    buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());

    // Serialize the random shares
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&prev_r_sh), reinterpret_cast<const uint8_t *>(&prev_r_sh) + sizeof(prev_r_sh));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&next_r_sh), reinterpret_cast<const uint8_t *>(&next_r_sh) + sizeof(next_r_sh));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&r), reinterpret_cast<const uint8_t *>(&r) + sizeof(r));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&r_sh_0), reinterpret_cast<const uint8_t *>(&r_sh_0) + sizeof(r_sh_0));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&r_sh_1), reinterpret_cast<const uint8_t *>(&r_sh_1) + sizeof(r_sh_1));
}

void OblivSelectKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing OblivSelectKey");
#endif
    size_t offset = 0;

    // Deserialize the party ID
    std::memcpy(&party_id, buffer.data() + offset, sizeof(party_id));
    offset += sizeof(party_id);

    // Deserialize the DPF keys
    size_t key_size = prev_key.CalculateSerializedSize();
    prev_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;
    key_size = next_key.CalculateSerializedSize();
    next_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;

    // Deserialize the random shares
    std::memcpy(&prev_r_sh, buffer.data() + offset, sizeof(prev_r_sh));
    offset += sizeof(prev_r_sh);
    std::memcpy(&next_r_sh, buffer.data() + offset, sizeof(next_r_sh));
    offset += sizeof(next_r_sh);
    std::memcpy(&r, buffer.data() + offset, sizeof(r));
    offset += sizeof(r);
    std::memcpy(&r_sh_0, buffer.data() + offset, sizeof(r_sh_0));
    offset += sizeof(r_sh_0);
    std::memcpy(&r_sh_1, buffer.data() + offset, sizeof(r_sh_1));
}

void OblivSelectKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("OblivSelect Key [Party " + std::to_string(party_id) + "]"));
        prev_key.PrintKey(true);
        next_key.PrintKey(true);
        Logger::DebugLog(LOC, "(prev_r_sh, next_r_sh): (" + std::to_string(prev_r_sh) + ", " + std::to_string(next_r_sh) + ")");
        Logger::DebugLog(LOC, "(r, r_sh_0, r_sh_1): (" + std::to_string(r) + ", " + std::to_string(r_sh_0) + ", " + std::to_string(r_sh_1) + ")");
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "OblivSelect Key [Party " + std::to_string(party_id) + "]");
        prev_key.PrintKey(false);
        next_key.PrintKey(false);
        Logger::DebugLog(LOC, "(prev_r_sh, next_r_sh): (" + std::to_string(prev_r_sh) + ", " + std::to_string(next_r_sh) + ")");
        Logger::DebugLog(LOC, "(r, r_sh_0, r_sh_1): (" + std::to_string(r) + ", " + std::to_string(r_sh_0) + ", " + std::to_string(r_sh_1) + ")");
    }
#endif
}

OblivSelectKeyGenerator::OblivSelectKeyGenerator(const OblivSelectParameters &params, sharing::ReplicatedSharing3P &rss, sharing::AdditiveSharing2P &ass)
    : params_(params), gen_(params.GetParameters()), rss_(rss), ass_(ass) {
}

std::array<OblivSelectKey, 3> OblivSelectKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<OblivSelectKey, 3> keys = {OblivSelectKey(0, params_), OblivSelectKey(1, params_), OblivSelectKey(2, params_)};

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generate OblivSelect Keys");
#endif

    // [Party 0]
    uint32_t                                      r_0    = rss_.GenerateRandomValue();
    std::pair<uint32_t, uint32_t>                 r_0_sh = ass_.Share(r_0);
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> key_0  = gen_.GenerateKeys(r_0, 1);
    // Send key_0.first and r_0_sh.first to Party 1
    // Send key_0.second and r_0_sh.second to Party 2
    // Set sampled value to own key
    keys[0].r      = r_0;
    keys[0].r_sh_0 = r_0_sh.first;
    keys[0].r_sh_1 = r_0_sh.second;

    // [Party 1]
    uint32_t                                      r_1    = rss_.GenerateRandomValue();
    std::pair<uint32_t, uint32_t>                 r_1_sh = ass_.Share(r_1);
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> key_1  = gen_.GenerateKeys(r_1, 1);
    // Send key_1.first and r_1_sh.first to Party 2
    // Send key_1.second and r_1_sh.second to Party 0
    // Set sampled value to own key
    keys[1].r      = r_1;
    keys[1].r_sh_0 = r_1_sh.first;
    keys[1].r_sh_1 = r_1_sh.second;

    // [Party 2]
    uint32_t                                      r_2    = rss_.GenerateRandomValue();
    std::pair<uint32_t, uint32_t>                 r_2_sh = ass_.Share(r_2);
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> key_2  = gen_.GenerateKeys(r_2, 1);
    // Send key_2.first and r_2_sh.first to Party 0
    // Send key_2.second and r_2_sh.second to Party 1
    // Set sampled value to own key
    keys[2].r      = r_2;
    keys[2].r_sh_0 = r_2_sh.first;
    keys[2].r_sh_1 = r_2_sh.second;

    // [Party 0]
    // Receive key_2.first and r_2_sh.first from Party 2
    // Receive key_1.second and r_1_sh.second from Party 1
    // Set key and shared value to own key
    keys[0].prev_key  = std::move(key_2.first);
    keys[0].prev_r_sh = r_2_sh.first;
    keys[0].next_key  = std::move(key_1.second);
    keys[0].next_r_sh = r_1_sh.second;

    // [Party 1]
    // Receive key_0.first and r_0_sh.first from Party 0
    // Receive key_2.second and r_2_sh.second from Party 2
    // Set key and shared value to own key
    keys[1].prev_key  = std::move(key_0.first);
    keys[1].prev_r_sh = r_0_sh.first;
    keys[1].next_key  = std::move(key_2.second);
    keys[1].next_r_sh = r_2_sh.second;

    // [Party 2]
    // Receive key_1.first and r_1_sh.first from Party 1
    // Receive key_0.second and r_0_sh.second from Party 0
    // Set key and shared value to own key
    keys[2].prev_key  = std::move(key_1.first);
    keys[2].prev_r_sh = r_1_sh.first;
    keys[2].next_key  = std::move(key_0.second);
    keys[2].next_r_sh = r_0_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    return keys;
}

OblivSelectEvaluator::OblivSelectEvaluator(const OblivSelectParameters &params, sharing::ReplicatedSharing3P &rss)
    : params_(params), eval_(params.GetParameters()), rss_(rss) {
}

void OblivSelectEvaluator::Evaluate(sharing::Channels         &chls,
                                    std::vector<uint32_t>     &uv_prev,
                                    std::vector<uint32_t>     &uv_next,
                                    const OblivSelectKey      &key,
                                    const sharing::SharesPair &database,
                                    const sharing::SharePair  &index,
                                    sharing::SharePair        &result) const {
    uint32_t d        = params_.GetDatabaseSize();
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
    index.DebugLog(party_id, "idx");
    database.DebugLog(party_id, "db");
#endif

    // Reconstruct p - r_i
    auto [pr_prev, pr_next] = ReconstructPR(chls, key, index, d);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + std::to_string(pr_prev) + ", pr_next: " + std::to_string(pr_next));
#endif

    // Evaluate DPF
    eval_.EvaluateFullDomain(key.prev_key, uv_prev);
    eval_.EvaluateFullDomain(key.next_key, uv_next);

    // Evaluate the Oblivious Selection
    uint32_t dp_prev = 0, dp_next = 0;
    for (uint32_t i = 0; i < uv_prev.size(); ++i) {
        dp_prev = Mod(dp_prev + (database.data[0][i] * uv_prev[Mod(i - pr_prev, d)]), d);
        dp_next = Mod(dp_next + (database.data[1][i] * uv_next[Mod(i - pr_next, d)]), d);
    }
    uint32_t           selected_sh = Mod(dp_prev + dp_next, d);
    sharing::SharePair r_sh;
    rss_.Rand(r_sh);
    result.data[0] = Mod(selected_sh + r_sh.data[0] - r_sh.data[1], d);
    chls.next.send(result.data[0]);
    chls.prev.recv(result.data[1]);
}

std::pair<uint32_t, uint32_t> OblivSelectEvaluator::ReconstructPR(sharing::Channels        &chls,
                                                                  const OblivSelectKey     &key,
                                                                  const sharing::SharePair &index,
                                                                  const uint32_t            d) const {
    Logger::DebugLog(LOC, "ReconstructPR for Party " + std::to_string(chls.party_id));

    // Set replicated sharing of random value
    uint32_t           pr_prev = 0, pr_next = 0;
    sharing::SharePair r_0_sh, r_1_sh, r_2_sh;

    switch (chls.party_id) {
        case 0:
            r_0_sh = sharing::SharePair(key.r_sh_0, key.r_sh_1);
            r_1_sh = sharing::SharePair(key.next_r_sh, 0);
            r_2_sh = sharing::SharePair(0, key.prev_r_sh);
            break;
        case 1:
            r_0_sh = sharing::SharePair(0, key.prev_r_sh);
            r_1_sh = sharing::SharePair(key.r_sh_0, key.r_sh_1);
            r_2_sh = sharing::SharePair(key.next_r_sh, 0);
            break;
        case 2:
            r_0_sh = sharing::SharePair(key.next_r_sh, 0);
            r_1_sh = sharing::SharePair(0, key.prev_r_sh);
            r_2_sh = sharing::SharePair(key.r_sh_0, key.r_sh_1);
            break;
        default:
            Logger::ErrorLog(LOC, "Invalid party_id: " + std::to_string(chls.party_id));
            return std::make_pair(0, 0);
    }

    // Reconstruct p - r_i
    sharing::SharePair p_r_sh;

    if (chls.party_id == 0) {
        // p - r_1 between Party 0 and Party 2
        rss_.EvaluateSub(index, r_1_sh, p_r_sh);
        chls.prev.send(p_r_sh.data[0]);
        uint32_t p_r_1_prev;
        chls.prev.recv(p_r_1_prev);
        pr_next = Mod(p_r_1_prev + p_r_sh.data[0] + p_r_sh.data[1], d);

        // p - r_2 between Party 0 and Party 1
        rss_.EvaluateSub(index, r_2_sh, p_r_sh);
        chls.next.send(p_r_sh.data[1]);
        uint32_t p_r_2_next;
        chls.next.recv(p_r_2_next);
        pr_prev = Mod(p_r_sh.data[0] + p_r_sh.data[1] + p_r_2_next, d);

    } else if (chls.party_id == 1) {
        // p - r_0 between Party 1 and Party 2
        rss_.EvaluateSub(index, r_0_sh, p_r_sh);
        chls.next.send(p_r_sh.data[1]);
        uint32_t p_r_0_next;
        chls.next.recv(p_r_0_next);
        pr_prev = Mod(p_r_sh.data[0] + p_r_sh.data[1] + p_r_0_next, d);

        // p - r_2 between Party 0 and Party 1
        rss_.EvaluateSub(index, r_2_sh, p_r_sh);
        uint32_t p_r_2_prev;
        chls.prev.recv(p_r_2_prev);
        chls.prev.send(p_r_sh.data[0]);
        pr_next = Mod(p_r_2_prev + p_r_sh.data[0] + p_r_sh.data[1], d);

    } else {
        // p - r_0 between Party 1 and Party 2
        rss_.EvaluateSub(index, r_0_sh, p_r_sh);
        uint32_t p_r_0_prev;
        chls.prev.recv(p_r_0_prev);
        chls.prev.send(p_r_sh.data[0]);
        pr_next = Mod(p_r_0_prev + p_r_sh.data[0] + p_r_sh.data[1], d);

        // p - r_1 between Party 0 and Party 2
        rss_.EvaluateSub(index, r_1_sh, p_r_sh);
        uint32_t p_r_1_next;
        chls.next.recv(p_r_1_next);
        chls.next.send(p_r_sh.data[1]);
        pr_prev = Mod(p_r_sh.data[0] + p_r_sh.data[1] + p_r_1_next, d);
    }
    return std::make_pair(pr_prev, pr_next);
}

}    // namespace wm
}    // namespace fsswm
