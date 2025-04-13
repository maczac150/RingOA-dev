#include "obliv_select.h"

#include <cstring>

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
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
      serialized_size_(CalculateSerializedSize()) {
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

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + std::to_string(buffer.size()) + " != " + std::to_string(serialized_size_));
        return;
    }
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
    size_t key_size = prev_key.GetSerializedSize();
    prev_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
    offset += key_size;
    key_size = next_key.GetSerializedSize();
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

OblivSelectKeyGenerator::OblivSelectKeyGenerator(
    const OblivSelectParameters &params,
    sharing::AdditiveSharing2P  &ass,
    sharing::BinarySharing2P    &bss)
    : params_(params),
      gen_(params.GetParameters()),
      ass_(ass), bss_(bss) {
}

std::array<OblivSelectKey, 3> OblivSelectKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<OblivSelectKey, 3> keys = {
        OblivSelectKey(0, params_),
        OblivSelectKey(1, params_),
        OblivSelectKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate OblivSelect Keys"));
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

void OblivSelectKeyGenerator::GenerateAdditiveKeys(std::array<OblivSelectKey, 3> &keys) const {
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

void OblivSelectKeyGenerator::GenerateBinaryKeys(std::array<OblivSelectKey, 3> &keys) const {
    std::array<uint32_t, 3>                                    rands;
    std::array<std::pair<uint32_t, uint32_t>, 3>               rand_shs;
    std::vector<std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey>> key_pairs;
    key_pairs.reserve(3);

    for (uint32_t i = 0; i < 3; ++i) {
        rands[i]    = bss_.GenerateRandomValue();
        rand_shs[i] = bss_.Share(rands[i]);
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

OblivSelectEvaluator::OblivSelectEvaluator(
    const OblivSelectParameters        &params,
    sharing::ReplicatedSharing3P       &rss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params), eval_(params.GetParameters()), rss_(rss), brss_(brss) {
}

void OblivSelectEvaluator::EvaluateAdditive(sharing::Channels          &chls,
                                            std::vector<uint32_t>      &uv_prev,
                                            std::vector<uint32_t>      &uv_next,
                                            const OblivSelectKey       &key,
                                            const sharing::RepShareVec &database,
                                            const sharing::RepShare    &index,
                                            sharing::RepShare          &result) const {

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
    auto [pr_prev, pr_next] = ReconstructPRAdditive(chls, key, index, d);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + std::to_string(pr_prev) + ", pr_next: " + std::to_string(pr_next));
#endif

    // Evaluate DPF
    eval_.EvaluateFullDomain(key.prev_key, uv_prev);
    eval_.EvaluateFullDomain(key.next_key, uv_next);

    // Evaluate the Oblivious Selection
    uint32_t dp_prev = 0, dp_next = 0;
    for (uint32_t i = 0; i < uv_prev.size(); ++i) {
        dp_prev = Mod(dp_prev + (database[0][i] * uv_prev[Mod(i - pr_prev, d)]), d);
        dp_next = Mod(dp_next + (database[1][i] * uv_next[Mod(i - pr_next, d)]), d);
    }
    uint32_t          selected_sh = Mod(dp_prev + dp_next, d);
    sharing::RepShare r_sh;
    rss_.Rand(r_sh);
    result[0] = Mod(selected_sh + r_sh[0] - r_sh[1], d);
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
}

std::pair<uint32_t, uint32_t> OblivSelectEvaluator::ReconstructPRAdditive(sharing::Channels       &chls,
                                                                          const OblivSelectKey    &key,
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
        chls.prev.send(p_r_sh[0]);
        uint32_t p_r_1_prev;
        chls.prev.recv(p_r_1_prev);
        pr_next = Mod(p_r_1_prev + p_r_sh[0] + p_r_sh[1], d);

        // p - r_2 between Party 0 and Party 1
        rss_.EvaluateSub(index, r_2_sh, p_r_sh);
        chls.next.send(p_r_sh[1]);
        uint32_t p_r_2_next;
        chls.next.recv(p_r_2_next);
        pr_prev = Mod(p_r_sh[0] + p_r_sh[1] + p_r_2_next, d);

    } else if (chls.party_id == 1) {
        // p - r_0 between Party 1 and Party 2
        rss_.EvaluateSub(index, r_0_sh, p_r_sh);
        chls.next.send(p_r_sh[1]);
        uint32_t p_r_0_next;
        chls.next.recv(p_r_0_next);
        pr_prev = Mod(p_r_sh[0] + p_r_sh[1] + p_r_0_next, d);

        // p - r_2 between Party 0 and Party 1
        rss_.EvaluateSub(index, r_2_sh, p_r_sh);
        uint32_t p_r_2_prev;
        chls.prev.recv(p_r_2_prev);
        chls.prev.send(p_r_sh[0]);
        pr_next = Mod(p_r_2_prev + p_r_sh[0] + p_r_sh[1], d);

    } else {
        // p - r_0 between Party 1 and Party 2
        rss_.EvaluateSub(index, r_0_sh, p_r_sh);
        uint32_t p_r_0_prev;
        chls.prev.recv(p_r_0_prev);
        chls.prev.send(p_r_sh[0]);
        pr_next = Mod(p_r_0_prev + p_r_sh[0] + p_r_sh[1], d);

        // p - r_1 between Party 0 and Party 2
        rss_.EvaluateSub(index, r_1_sh, p_r_sh);
        uint32_t p_r_1_next;
        chls.next.recv(p_r_1_next);
        chls.next.send(p_r_sh[1]);
        pr_prev = Mod(p_r_sh[0] + p_r_sh[1] + p_r_1_next, d);
    }
    return std::make_pair(pr_prev, pr_next);
}

void OblivSelectEvaluator::EvaluateBinary(sharing::Channels          &chls,
                                          std::vector<block>         &uv_prev,
                                          std::vector<block>         &uv_next,
                                          const OblivSelectKey       &key,
                                          const sharing::RepShareVec &database,
                                          const sharing::RepShare    &index,
                                          sharing::RepShare          &result) const {

    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
    index.DebugLog(party_id, "idx");
    database.DebugLog(party_id, "db");
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructPRBinary(chls, key, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + std::to_string(pr_prev) + ", pr_next: " + std::to_string(pr_next));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block == __m128i)
    eval_.EvaluateFullDomainOneBit(key.prev_key, uv_prev);
    eval_.EvaluateFullDomainOneBit(key.next_key, uv_next);

    // Now use uv_prev_bits and uv_next_bits in place of the block vectors:
    uint32_t dp_prev = 0, dp_next = 0;
    for (size_t i = 0; i < database.num_shares; ++i) {
        size_t block_index = i / 128;
        size_t bit_index   = i % 128;
        size_t byte_index  = bit_index / 8;
        size_t bit_in_byte = bit_index % 8;

        alignas(16) uint8_t buffer_prev[16], buffer_next[16];
        _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer_prev), uv_prev[block_index]);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer_next), uv_next[block_index]);

        uint8_t bit_prev = (buffer_prev[byte_index] >> bit_in_byte) & 1;
        uint8_t bit_next = (buffer_next[byte_index] >> bit_in_byte) & 1;

        dp_prev ^= database[0][i ^ pr_prev] * bit_prev;
        dp_next ^= database[1][i ^ pr_next] * bit_next;
    }

    uint32_t          selected_sh = dp_prev ^ dp_next;
    sharing::RepShare r_sh;
    brss_.Rand(r_sh);
    result[0] = selected_sh ^ r_sh[0] ^ r_sh[1];
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
}

void OblivSelectEvaluator::EvaluateBinary(sharing::Channels              &chls,
                                          std::vector<block>             &uv_prev,
                                          std::vector<block>             &uv_next,
                                          const OblivSelectKey           &key,
                                          const sharing::RepShareVecView &database1,
                                          const sharing::RepShareVecView &database2,
                                          const sharing::RepShare        &index,
                                          sharing::RepShare              &result1,
                                          sharing::RepShare              &result2) const {

    uint32_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + std::to_string(party_id));
    std::string party_str = "[P" + std::to_string(party_id) + "] ";
    index.DebugLog(party_id, "idx");
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructPRBinary(chls, key, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + std::to_string(pr_prev) + ", pr_next: " + std::to_string(pr_next));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block == __m128i)
    eval_.EvaluateFullDomainOneBit(key.prev_key, uv_prev);
    eval_.EvaluateFullDomainOneBit(key.next_key, uv_next);

    // Now use uv_prev_bits and uv_next_bits in place of the block vectors:
    uint32_t dp_prev_1 = 0, dp_next_1 = 0, dp_prev_2 = 0, dp_next_2 = 0;
    for (size_t i = 0; i < database1.num_shares(); ++i) {
        size_t block_index = i / 128;
        size_t bit_index   = i % 128;
        size_t byte_index  = bit_index / 8;
        size_t bit_in_byte = bit_index % 8;

        alignas(16) uint8_t buffer_prev[16], buffer_next[16];
        _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer_prev), uv_prev[block_index]);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(buffer_next), uv_next[block_index]);

        uint8_t bit_prev = (buffer_prev[byte_index] >> bit_in_byte) & 1;
        uint8_t bit_next = (buffer_next[byte_index] >> bit_in_byte) & 1;

        dp_prev_1 ^= (database1[0][i ^ pr_prev] * bit_prev);
        dp_next_1 ^= (database1[1][i ^ pr_next] * bit_next);
        dp_prev_2 ^= (database2[0][i ^ pr_prev] * bit_prev);
        dp_next_2 ^= (database2[1][i ^ pr_next] * bit_next);
    }

    uint32_t          selected_sh_1 = dp_prev_1 ^ dp_next_1;
    uint32_t          selected_sh_2 = dp_prev_2 ^ dp_next_2;
    sharing::RepShare r_sh;
    brss_.Rand(r_sh);
    result1[0] = selected_sh_1 ^ r_sh[0] ^ r_sh[1];
    chls.next.send(result1[0]);
    chls.prev.recv(result1[1]);
    brss_.Rand(r_sh);
    result2[0] = selected_sh_2 ^ r_sh[0] ^ r_sh[1];
    chls.next.send(result2[0]);
    chls.prev.recv(result2[1]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " result1: " + std::to_string(result1[0]) + ", " + std::to_string(result1[1]));
    Logger::DebugLog(LOC, party_str + " result2: " + std::to_string(result2[0]) + ", " + std::to_string(result2[1]));
#endif
}

std::pair<uint32_t, uint32_t> OblivSelectEvaluator::ReconstructPRBinary(sharing::Channels       &chls,
                                                                        const OblivSelectKey    &key,
                                                                        const sharing::RepShare &index) const {

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "ReconstructPR for Party " + std::to_string(chls.party_id));
#endif

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
        brss_.EvaluateXor(index, r_1_sh, p_r_sh);
        chls.prev.send(p_r_sh[0]);
        uint32_t p_r_1_prev;
        chls.prev.recv(p_r_1_prev);
        pr_next = p_r_1_prev ^ p_r_sh[0] ^ p_r_sh[1];

        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_2_sh, p_r_sh);
        chls.next.send(p_r_sh[1]);
        uint32_t p_r_2_next;
        chls.next.recv(p_r_2_next);
        pr_prev = p_r_sh[0] ^ p_r_sh[1] ^ p_r_2_next;

    } else if (chls.party_id == 1) {
        // p ^ r_0 between Party 1 and Party 2
        brss_.EvaluateXor(index, r_0_sh, p_r_sh);
        chls.next.send(p_r_sh[1]);
        uint32_t p_r_0_next;
        chls.next.recv(p_r_0_next);
        pr_prev = p_r_sh[0] ^ p_r_sh[1] ^ p_r_0_next;

        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_2_sh, p_r_sh);
        uint32_t p_r_2_prev;
        chls.prev.recv(p_r_2_prev);
        chls.prev.send(p_r_sh[0]);
        pr_next = p_r_2_prev ^ p_r_sh[0] ^ p_r_sh[1];

    } else {
        // p ^ r_0 between Party 1 and Party 2
        brss_.EvaluateXor(index, r_0_sh, p_r_sh);
        uint32_t p_r_0_prev;
        chls.prev.recv(p_r_0_prev);
        chls.prev.send(p_r_sh[0]);
        pr_next = p_r_0_prev ^ p_r_sh[0] ^ p_r_sh[1];

        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(index, r_1_sh, p_r_sh);
        uint32_t p_r_1_next;
        chls.next.recv(p_r_1_next);
        chls.next.send(p_r_sh[1]);
        pr_prev = p_r_sh[0] ^ p_r_sh[1] ^ p_r_1_next;
    }
    return std::make_pair(pr_prev, pr_next);
}

}    // namespace wm
}    // namespace fsswm
