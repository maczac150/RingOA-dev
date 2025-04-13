#include "zero_test.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/utils/logger.h"

namespace fsswm {
namespace fm_index {

void ZeroTestParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Zero Test Parameters]" + GetParametersInfo());
}

ZeroTestKey::ZeroTestKey(const uint32_t id, const ZeroTestParameters &params)
    : party_id(id),
      prev_key(0, params.GetParameters()),
      next_key(1, params.GetParameters()),
      prev_r_sh(0), next_r_sh(0),
      r(0), r_sh_0(0), r_sh_1(0),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
}

size_t ZeroTestKey::CalculateSerializedSize() const {
    return sizeof(party_id) + prev_key.GetSerializedSize() + next_key.GetSerializedSize() +
           sizeof(prev_r_sh) + sizeof(next_r_sh) +
           sizeof(r) + sizeof(r_sh_0) + sizeof(r_sh_1);
}

void ZeroTestKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing Zero Test Key");
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

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + std::to_string(buffer.size()) + " != " + std::to_string(serialized_size_));
        return;
    }
}

void ZeroTestKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing Zero Test Key");
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

void ZeroTestKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("Zero Test Key [Party " + std::to_string(party_id) + "]"));
        prev_key.PrintKey(true);
        next_key.PrintKey(true);
        Logger::DebugLog(LOC, "(prev_r_sh, next_r_sh): (" + std::to_string(prev_r_sh) + ", " + std::to_string(next_r_sh) + ")");
        Logger::DebugLog(LOC, "(r, r_sh_0, r_sh_1): (" + std::to_string(r) + ", " + std::to_string(r_sh_0) + ", " + std::to_string(r_sh_1) + ")");
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "Zero Test Key [Party " + std::to_string(party_id) + "]");
        prev_key.PrintKey(false);
        next_key.PrintKey(false);
        Logger::DebugLog(LOC, "(prev_r_sh, next_r_sh): (" + std::to_string(prev_r_sh) + ", " + std::to_string(next_r_sh) + ")");
        Logger::DebugLog(LOC, "(r, r_sh_0, r_sh_1): (" + std::to_string(r) + ", " + std::to_string(r_sh_0) + ", " + std::to_string(r_sh_1) + ")");
    }
#endif
}

ZeroTestKeyGenerator::ZeroTestKeyGenerator(
    const ZeroTestParameters   &params,
    sharing::AdditiveSharing2P &ass,
    sharing::BinarySharing2P   &bss)
    : params_(params), gen_(params.GetParameters()), ass_(ass), bss_(bss) {
}

std::array<ZeroTestKey, 3> ZeroTestKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<ZeroTestKey, 3> keys = {
        ZeroTestKey(0, params_),
        ZeroTestKey(1, params_),
        ZeroTestKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generate ZeroTest Keys");
#endif

    switch (params_.GetType()) {
        case ShareType::kAdditive:
            GenerateAdditiveKeys(keys);
            break;
        case ShareType::kBinary:
            GenerateBinaryKeys(keys);
            break;
        default:
            Logger::ErrorLog(LOC, "Unsupported ShareType");
            break;
    }
    return keys;
}

void ZeroTestKeyGenerator::GenerateAdditiveKeys(std::array<ZeroTestKey, 3> &keys) const {
    std::array<uint32_t, 3>                                    rands;
    std::array<std::pair<uint32_t, uint32_t>, 3>               rand_shs;
    std::vector<std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey>> key_pairs;
    key_pairs.reserve(3);

    for (uint32_t i = 0; i < 3; ++i) {
        rands[i]    = ass_.GenerateRandomValue();
        rand_shs[i] = ass_.Share(rands[i]);
        key_pairs.push_back(gen_.GenerateKeys(rands[i], 1));

        keys[i].r      = rands[i];
        keys[i].r_sh_0 = rand_shs[i].first;
        keys[i].r_sh_1 = rand_shs[i].second;
    }

    // Assign previous and next keys
    for (uint32_t i = 0; i < 3; ++i) {
        uint32_t prev     = (i + 2) % 3;
        uint32_t next     = (i + 1) % 3;
        keys[i].prev_key  = std::move(key_pairs[prev].first);
        keys[i].prev_r_sh = rand_shs[prev].first;
        keys[i].next_key  = std::move(key_pairs[next].second);
        keys[i].next_r_sh = rand_shs[next].second;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif
}

void ZeroTestKeyGenerator::GenerateBinaryKeys(std::array<ZeroTestKey, 3> &keys) const {
    std::array<uint32_t, 3>                                    rands;
    std::array<std::pair<uint32_t, uint32_t>, 3>               rand_shs;
    std::vector<std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey>> key_pairs;
    key_pairs.reserve(sharing::kNumParties);

    for (uint32_t i = 0; i < 3; ++i) {
        rands[i]    = bss_.GenerateRandomValue();
        rand_shs[i] = bss_.Share(rands[i]);
        key_pairs.push_back(gen_.GenerateKeys(rands[i], 1));
        key_pairs[i] = gen_.GenerateKeys(rands[i], 1);

        keys[i].r      = rands[i];
        keys[i].r_sh_0 = rand_shs[i].first;
        keys[i].r_sh_1 = rand_shs[i].second;
    }

    // Assign previous and next keys
    for (uint32_t i = 0; i < 3; ++i) {
        uint32_t prev     = (i + 2) % 3;
        uint32_t next     = (i + 1) % 3;
        keys[i].prev_key  = std::move(key_pairs[prev].first);
        keys[i].prev_r_sh = rand_shs[prev].first;
        keys[i].next_key  = std::move(key_pairs[next].second);
        keys[i].next_r_sh = rand_shs[next].second;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif
}

ZeroTestEvaluator::ZeroTestEvaluator(
    const ZeroTestParameters           &params,
    sharing::ReplicatedSharing3P       &rss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params), eval_(params.GetParameters()), rss_(rss), brss_(brss) {
}

void ZeroTestEvaluator::EvaluateAdditive(sharing::Channels       &chls,
                                         const ZeroTestKey       &key,
                                         const sharing::RepShare &x,
                                         sharing::RepShare       &result) const {
    uint32_t n        = params_.GetParameters().GetInputBitsize();
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate ZeroTest key"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
    x.DebugLog(party_id, "idx");
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructPRAdditive(chls, key, x, n);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + std::to_string(pr_prev) + ", pr_next: " + std::to_string(pr_next));
#endif

    // Evaluate DPF
    uint32_t dpf_prev = eval_.EvaluateAt(key.prev_key, pr_prev);
    uint32_t dpf_next = eval_.EvaluateAt(key.next_key, pr_next);

    uint32_t          res_sh = Mod(dpf_prev * dpf_next, n);
    sharing::RepShare r_sh;
    rss_.Rand(r_sh);
    result.data[0] = Mod(res_sh + r_sh.data[0] - r_sh.data[1], n);
    chls.next.send(result.data[0]);
    chls.prev.recv(result.data[1]);
}

std::pair<uint32_t, uint32_t> ZeroTestEvaluator::ReconstructPRAdditive(sharing::Channels       &chls,
                                                                       const ZeroTestKey       &key,
                                                                       const sharing::RepShare &index,
                                                                       const uint32_t           d) const {
    Logger::DebugLog(LOC, "ReconstructPR for Party " + std::to_string(chls.party_id));

    // Set replicated sharing of random value
    uint32_t          pr_prev = 0, pr_next = 0;
    sharing::RepShare r_0_sh, r_1_sh, r_2_sh;

    switch (chls.party_id) {
        case 0:
            r_0_sh = sharing::RepShare(key.r_sh_0, key.r_sh_1);
            r_1_sh = sharing::RepShare(key.next_r_sh, 0);
            r_2_sh = sharing::RepShare(0, key.prev_r_sh);
            break;
        case 1:
            r_0_sh = sharing::RepShare(0, key.prev_r_sh);
            r_1_sh = sharing::RepShare(key.r_sh_0, key.r_sh_1);
            r_2_sh = sharing::RepShare(key.next_r_sh, 0);
            break;
        case 2:
            r_0_sh = sharing::RepShare(key.next_r_sh, 0);
            r_1_sh = sharing::RepShare(0, key.prev_r_sh);
            r_2_sh = sharing::RepShare(key.r_sh_0, key.r_sh_1);
            break;
        default:
            Logger::ErrorLog(LOC, "Invalid party_id: " + std::to_string(chls.party_id));
            return std::make_pair(0, 0);
    }

    // Reconstruct p - r_i
    sharing::RepShare p_r_sh;

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

void ZeroTestEvaluator::EvaluateBinary(sharing::Channels       &chls,
                                       const ZeroTestKey       &key,
                                       const sharing::RepShare &x,
                                       sharing::RepShare       &result) const {
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate ZeroTest key"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
    x.DebugLog(party_id, "idx");
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructPRBinary(chls, key, x);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + std::to_string(pr_prev) + ", pr_next: " + std::to_string(pr_next));
#endif

    // Evaluate DPF
    uint32_t dpf_prev = eval_.EvaluateAt(key.prev_key, pr_prev);
    uint32_t dpf_next = eval_.EvaluateAt(key.next_key, pr_next);

    uint32_t          res_sh = dpf_prev ^ dpf_next;
    sharing::RepShare r_sh;
    brss_.Rand(r_sh);
    result.data[0] = res_sh ^ r_sh.data[0] ^ r_sh.data[1];
    chls.next.send(result.data[0]);
    chls.prev.recv(result.data[1]);
}

void ZeroTestEvaluator::EvaluateBinary(sharing::Channels              &chls,
                                       const std::vector<ZeroTestKey> &key,
                                       const sharing::RepShareVec     &x,
                                       sharing::RepShareVec           &result) const {
    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate ZeroTest key"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
    x.DebugLog(party_id, "idx");
#endif

    std::vector<uint32_t> pr_prev(x.num_shares), pr_next(x.num_shares);
    ReconstructPRBinary(chls, key, x, pr_prev, pr_next);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + ToString(pr_prev) + ", pr_next: " + ToString(pr_next));
#endif

    for (uint32_t i = 0; i < x.num_shares; ++i) {
        uint32_t dpf_prev = eval_.EvaluateAt(key[i].prev_key, pr_prev[i]);
        uint32_t dpf_next = eval_.EvaluateAt(key[i].next_key, pr_next[i]);

        uint32_t          res_sh = dpf_prev ^ dpf_next;
        sharing::RepShare r_sh;
        brss_.Rand(r_sh);
        result.data[0][i] = res_sh ^ r_sh.data[0] ^ r_sh.data[1];
    }
    chls.next.send(result.data[0]);
    chls.prev.recv(result.data[1]);
}

std::pair<uint32_t, uint32_t> ZeroTestEvaluator::ReconstructPRBinary(sharing::Channels       &chls,
                                                                     const ZeroTestKey       &key,
                                                                     const sharing::RepShare &x) const {
    Logger::DebugLog(LOC, "ReconstructPR for Party " + std::to_string(chls.party_id));

    // Set replicated sharing of random value
    uint32_t          pr_prev = 0, pr_next = 0;
    sharing::RepShare r_0_sh, r_1_sh, r_2_sh;

    switch (chls.party_id) {
        case 0:
            r_0_sh = sharing::RepShare(key.r_sh_0, key.r_sh_1);
            r_1_sh = sharing::RepShare(key.next_r_sh, 0);
            r_2_sh = sharing::RepShare(0, key.prev_r_sh);
            break;
        case 1:
            r_0_sh = sharing::RepShare(0, key.prev_r_sh);
            r_1_sh = sharing::RepShare(key.r_sh_0, key.r_sh_1);
            r_2_sh = sharing::RepShare(key.next_r_sh, 0);
            break;
        case 2:
            r_0_sh = sharing::RepShare(key.next_r_sh, 0);
            r_1_sh = sharing::RepShare(0, key.prev_r_sh);
            r_2_sh = sharing::RepShare(key.r_sh_0, key.r_sh_1);
            break;
        default:
            Logger::ErrorLog(LOC, "Invalid party_id: " + std::to_string(chls.party_id));
            return std::make_pair(0, 0);
    }

    // Reconstruct p ^ r_i
    sharing::RepShare p_r_sh;

    if (chls.party_id == 0) {
        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(x, r_1_sh, p_r_sh);
        chls.prev.send(p_r_sh.data[0]);
        uint32_t p_r_1_prev;
        chls.prev.recv(p_r_1_prev);
        pr_next = p_r_1_prev ^ p_r_sh.data[0] ^ p_r_sh.data[1];

        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(x, r_2_sh, p_r_sh);
        chls.next.send(p_r_sh.data[1]);
        uint32_t p_r_2_next;
        chls.next.recv(p_r_2_next);
        pr_prev = p_r_sh.data[0] ^ p_r_sh.data[1] ^ p_r_2_next;

    } else if (chls.party_id == 1) {
        // p ^ r_0 between Party 1 and Party 2
        brss_.EvaluateXor(x, r_0_sh, p_r_sh);
        chls.next.send(p_r_sh.data[1]);
        uint32_t p_r_0_next;
        chls.next.recv(p_r_0_next);
        pr_prev = p_r_sh.data[0] ^ p_r_sh.data[1] ^ p_r_0_next;

        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(x, r_2_sh, p_r_sh);
        uint32_t p_r_2_prev;
        chls.prev.recv(p_r_2_prev);
        chls.prev.send(p_r_sh.data[0]);
        pr_next = p_r_2_prev ^ p_r_sh.data[0] ^ p_r_sh.data[1];

    } else {
        // p ^ r_0 between Party 1 and Party 2
        brss_.EvaluateXor(x, r_0_sh, p_r_sh);
        uint32_t p_r_0_prev;
        chls.prev.recv(p_r_0_prev);
        chls.prev.send(p_r_sh.data[0]);
        pr_next = p_r_0_prev ^ p_r_sh.data[0] ^ p_r_sh.data[1];

        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(x, r_1_sh, p_r_sh);
        uint32_t p_r_1_next;
        chls.next.recv(p_r_1_next);
        chls.next.send(p_r_sh.data[1]);
        pr_prev = p_r_sh.data[0] ^ p_r_sh.data[1] ^ p_r_1_next;
    }
    return std::make_pair(pr_prev, pr_next);
}

void ZeroTestEvaluator::ReconstructPRBinary(sharing::Channels              &chls,
                                            const std::vector<ZeroTestKey> &key,
                                            const sharing::RepShareVec     &x,
                                            std::vector<uint32_t>          &res_prev,
                                            std::vector<uint32_t>          &res_next) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "ReconstructPR for Party " + std::to_string(chls.party_id));
#endif

    if (res_prev.size() != x.num_shares) {
        res_prev.resize(x.num_shares);
    }
    if (res_next.size() != x.num_shares) {
        res_next.resize(x.num_shares);
    }

    // Set replicated sharing of random value
    sharing::RepShareVec r_0_sh(x.num_shares), r_1_sh(x.num_shares), r_2_sh(x.num_shares);

    for (uint32_t i = 0; i < x.num_shares; ++i) {
        switch (chls.party_id) {
            case 0:
                r_0_sh.Set(i, sharing::RepShare(key[i].r_sh_0, key[i].r_sh_1));
                r_1_sh.Set(i, sharing::RepShare(key[i].next_r_sh, 0));
                r_2_sh.Set(i, sharing::RepShare(0, key[i].prev_r_sh));
                break;
            case 1:
                r_0_sh.Set(i, sharing::RepShare(0, key[i].prev_r_sh));
                r_1_sh.Set(i, sharing::RepShare(key[i].r_sh_0, key[i].r_sh_1));
                r_2_sh.Set(i, sharing::RepShare(key[i].next_r_sh, 0));
                break;
            case 2:
                r_0_sh.Set(i, sharing::RepShare(key[i].next_r_sh, 0));
                r_1_sh.Set(i, sharing::RepShare(0, key[i].prev_r_sh));
                r_2_sh.Set(i, sharing::RepShare(key[i].r_sh_0, key[i].r_sh_1));
                break;
            default:
                Logger::ErrorLog(LOC, "Invalid party_id: " + std::to_string(chls.party_id));
        }
    }

    // Reconstruct p ^ r_i
    sharing::RepShareVec p_r_sh(x.num_shares);

    if (chls.party_id == 0) {
        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(x, r_1_sh, p_r_sh);
        chls.prev.send(p_r_sh.data[0]);
        std::vector<uint32_t> p_r_1_prev;
        chls.prev.recv(p_r_1_prev);
        for (uint32_t i = 0; i < x.num_shares; ++i) {
            res_next[i] = p_r_1_prev[i] ^ p_r_sh.data[0][i] ^ p_r_sh.data[1][i];
        }

        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(x, r_2_sh, p_r_sh);
        chls.next.send(p_r_sh.data[1]);
        std::vector<uint32_t> p_r_2_next;
        chls.next.recv(p_r_2_next);
        for (uint32_t i = 0; i < x.num_shares; ++i) {
            res_prev[i] = p_r_sh.data[0][i] ^ p_r_sh.data[1][i] ^ p_r_2_next[i];
        }

    } else if (chls.party_id == 1) {
        // p ^ r_0 between Party 1 and Party 2
        brss_.EvaluateXor(x, r_0_sh, p_r_sh);
        chls.next.send(p_r_sh.data[1]);
        std::vector<uint32_t> p_r_0_next;
        chls.next.recv(p_r_0_next);
        for (uint32_t i = 0; i < x.num_shares; ++i) {
            res_prev[i] = p_r_sh.data[0][i] ^ p_r_sh.data[1][i] ^ p_r_0_next[i];
        }

        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(x, r_2_sh, p_r_sh);
        std::vector<uint32_t> p_r_2_prev;
        chls.prev.recv(p_r_2_prev);
        chls.prev.send(p_r_sh.data[0]);
        for (uint32_t i = 0; i < x.num_shares; ++i) {
            res_next[i] = p_r_2_prev[i] ^ p_r_sh.data[0][i] ^ p_r_sh.data[1][i];
        }

    } else {
        // p ^ r_0 between Party 1 and Party 2
        brss_.EvaluateXor(x, r_0_sh, p_r_sh);
        std::vector<uint32_t> p_r_0_prev;
        chls.prev.recv(p_r_0_prev);
        chls.prev.send(p_r_sh.data[0]);
        for (uint32_t i = 0; i < x.num_shares; ++i) {
            res_next[i] = p_r_0_prev[i] ^ p_r_sh.data[0][i] ^ p_r_sh.data[1][i];
        }

        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(x, r_1_sh, p_r_sh);
        std::vector<uint32_t> p_r_1_next;
        chls.next.recv(p_r_1_next);
        chls.next.send(p_r_sh.data[1]);
        for (uint32_t i = 0; i < x.num_shares; ++i) {
            res_prev[i] = p_r_sh.data[0][i] ^ p_r_sh.data[1][i] ^ p_r_1_next[i];
        }
    }
}

}    // namespace fm_index
}    // namespace fsswm
