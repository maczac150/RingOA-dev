#include "obliv_select.h"

#include <cstring>

#include "FssWM/fss/prg.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
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
    : params_(params), eval_(params.GetParameters()), rss_(rss), brss_(brss),
      G_(fss::prg::PseudoRandomGeneratorSingleton::GetInstance()) {
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
    // eval_.EvaluateFullDomain(key.prev_key, uv_prev);
    // eval_.EvaluateFullDomain(key.next_key, uv_next);
    // auto [dp_prev, dp_next] = BinaryDotProduct(uv_prev, uv_next, pr_prev, pr_next, database);
    uint32_t dp_prev = FullDomainDotProduct(key.prev_key, database[0], pr_prev);
    uint32_t dp_next = FullDomainDotProduct(key.next_key, database[1], pr_next);

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
    eval_.EvaluateFullDomain(key.prev_key, uv_prev);
    eval_.EvaluateFullDomain(key.next_key, uv_next);

    // Now use uv_prev_bits and uv_next_bits in place of the block vectors:
    uint32_t dp_prev_1 = 0, dp_next_1 = 0, dp_prev_2 = 0, dp_next_2 = 0;
    auto    *uv_prev_bytes = reinterpret_cast<const uint8_t *>(uv_prev.data());
    auto    *uv_next_bytes = reinterpret_cast<const uint8_t *>(uv_next.data());

    for (size_t i = 0; i < database1.num_shares(); ++i) {
        size_t block_index = i / 128;
        size_t bit_index   = i % 128;
        size_t byte_index  = bit_index / 8;
        size_t bit_in_byte = bit_index % 8;

        size_t  offset    = block_index * 16 + byte_index;
        uint8_t byte_prev = uv_prev_bytes[offset];
        uint8_t byte_next = uv_next_bytes[offset];
        // TODO: 無駄にやってるから内側に8個のループを作る
        // TODO: それが無理ならDPF側と同時にやる

        const uint32_t bit_prev = (byte_prev >> bit_in_byte) & 1u;
        const uint32_t bit_next = (byte_next >> bit_in_byte) & 1u;

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
    sharing::RepShare pr_prev_sh;
    sharing::RepShare pr_next_sh;

    if (chls.party_id == 0) {
        // p ^ r_1 between Party 0 and Party 2
        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_1_sh, pr_prev_sh);
        brss_.EvaluateXor(index, r_2_sh, pr_next_sh);
        chls.prev.send(pr_prev_sh[0]);
        chls.next.send(pr_next_sh[1]);
        uint32_t p_r_1_prev;
        uint32_t p_r_2_next;
        chls.prev.recv(p_r_1_prev);
        chls.next.recv(p_r_2_next);
        pr_next = p_r_1_prev ^ pr_prev_sh[0] ^ pr_prev_sh[1];
        pr_prev = pr_next_sh[0] ^ pr_next_sh[1] ^ p_r_2_next;

    } else if (chls.party_id == 1) {
        // p ^ r_0 between Party 1 and Party 2
        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_0_sh, pr_next_sh);
        brss_.EvaluateXor(index, r_2_sh, pr_prev_sh);
        chls.next.send(pr_next_sh[1]);
        chls.prev.send(pr_prev_sh[0]);
        uint32_t p_r_0_next;
        uint32_t p_r_2_prev;
        chls.next.recv(p_r_0_next);
        chls.prev.recv(p_r_2_prev);
        pr_next = p_r_2_prev ^ pr_prev_sh[0] ^ pr_prev_sh[1];
        pr_prev = pr_next_sh[0] ^ pr_next_sh[1] ^ p_r_0_next;

    } else {
        // p ^ r_0 between Party 1 and Party 2
        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(index, r_0_sh, pr_prev_sh);
        brss_.EvaluateXor(index, r_1_sh, pr_next_sh);
        chls.prev.send(pr_prev_sh[0]);
        chls.next.send(pr_next_sh[1]);

        uint32_t p_r_0_prev;
        uint32_t p_r_1_next;
        chls.prev.recv(p_r_0_prev);
        chls.next.recv(p_r_1_next);
        pr_next = p_r_0_prev ^ pr_prev_sh[0] ^ pr_prev_sh[1];
        pr_prev = pr_next_sh[0] ^ pr_next_sh[1] ^ p_r_1_next;
    }
    return std::make_pair(pr_prev, pr_next);
}

std::pair<uint32_t, uint32_t> OblivSelectEvaluator::BinaryDotProduct(
    std::vector<block>         &uv_prev,
    std::vector<block>         &uv_next,
    const uint32_t              pr_prev,
    const uint32_t              pr_next,
    const sharing::RepShareVec &database) const {
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
    // =================================================
    // Optimized bit extraction + mask-based accumulation

    // uint32_t dp_prev = 0, dp_next = 0;
    // size_t   num_blocks = uv_prev.size();    // = num_shares / 128

    // for (size_t block = 0; block < num_blocks; ++block) {
    //     // Load one 128-bit block into registers
    //     __m128i prev_block = uv_prev[block];
    //     __m128i next_block = uv_next[block];

    //     // Split into two 64-bit lanes
    //     uint64_t prev_low  = static_cast<uint64_t>(_mm_cvtsi128_si64(prev_block));
    //     uint64_t prev_high = static_cast<uint64_t>(_mm_extract_epi64(prev_block, 1));
    //     uint64_t next_low  = static_cast<uint64_t>(_mm_cvtsi128_si64(next_block));
    //     uint64_t next_high = static_cast<uint64_t>(_mm_extract_epi64(next_block, 1));

    //     // Process all 128 bits in this block
    //     for (int bit = 0; bit < 128; ++bit) {
    //         size_t idx = block * 128 + bit;

    //         // Compute permuted index
    //         size_t db_idx_prev = idx ^ pr_prev;
    //         size_t db_idx_next = idx ^ pr_next;

    //         // Extract bit_prev/bit_next by shifting the right 64-bit lane
    //         uint32_t bit_prev = static_cast<uint32_t>(
    //             ((bit < 64 ? (prev_low >> bit) : (prev_high >> (bit - 64))) & 1));
    //         uint32_t bit_next = static_cast<uint32_t>(
    //             ((bit < 64 ? (next_low >> bit) : (next_high >> (bit - 64))) & 1));

    //         // Avoid multiply: build a 0 or 0xFFFFFFFF mask from the bit
    //         uint32_t mask_prev = 0u - bit_prev;
    //         uint32_t mask_next = 0u - bit_next;

    //         // XOR in only the selected database entry
    //         dp_prev ^= (database.data[0][db_idx_prev] & mask_prev);
    //         dp_next ^= (database.data[1][db_idx_next] & mask_next);
    //     }
    // }
    return std::make_pair(dp_prev, dp_next);
    // =================================================
}

uint32_t OblivSelectEvaluator::FullDomainDotProduct(const fss::dpf::DpfKey      &key,
                                                    const std::vector<uint32_t> &database,
                                                    const uint32_t               pr) const {
    uint32_t nu            = params_.GetParameters().GetTerminateBitsize();
    uint32_t remaining_bit = params_.GetParameters().GetInputBitsize() - nu;

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint32_t i = 0; i < 3; ++i) {
        alignas(16) std::array<block, 2> expanded_seeds;
        std::array<bool, 2>              expanded_control_bits;
        next_seeds.resize(1U << (i + 1));
        next_control_bits.resize(1U << (i + 1));
        for (size_t j = 0; j < start_seeds.size(); j++) {
            EvaluateNextSeed(i, start_seeds[j], start_control_bits[j], expanded_seeds, expanded_control_bits, key);
            next_seeds[j * 2]            = expanded_seeds[fss::kLeft];
            next_seeds[j * 2 + 1]        = expanded_seeds[fss::kRight];
            next_control_bits[j * 2]     = expanded_control_bits[fss::kLeft];
            next_control_bits[j * 2 + 1] = expanded_control_bits[fss::kRight];
        }
        start_seeds        = std::move(next_seeds);
        start_control_bits = std::move(next_control_bits);
    }

    // Initialize the variables
    uint32_t current_level = 0;
    uint32_t current_idx   = 0;
    uint32_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint32_t last_idx      = 1U << last_depth;
    uint32_t dp            = 0;

    // Store the seeds and control bits
    std::array<block, 8>              expanded_seeds;
    // uint8_t                          *seeds_view = reinterpret_cast<uint8_t *>(expanded_seeds.data());
    std::array<bool, 8>               expanded_control_bits;
    std::vector<std::array<block, 8>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 8>>  prev_control_bits(last_depth + 1);

    // Evaluate the DPF key
    for (uint32_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint32_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint32_t i = 0; i < 8; ++i) {
                expanded_control_bits[i] = getLSB(expanded_seeds[i]);
            }

#if LOG_LEVEL >= LOG_LEVEL_TRACE
            std::string level_str = "|Level=" + std::to_string(current_level) + "| ";
            for (uint32_t i = 0; i < 8; ++i) {
                Logger::TraceLog(LOC, level_str + "Current bit: " + std::to_string(current_bit));
                Logger::TraceLog(LOC, level_str + "Current seed (" + std::to_string(i) + "): " + ToString(prev_seeds[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Current control bit (" + std::to_string(i) + "): " + std::to_string(prev_control_bits[current_level][i]));
                Logger::TraceLog(LOC, level_str + "Expanded seed (" + std::to_string(i) + "): " + ToString(expanded_seeds[i]));
                Logger::TraceLog(LOC, level_str + "Expanded control bit (" + std::to_string(i) + "): " + std::to_string(expanded_control_bits[i]));
            }
#endif

            // Apply correction word if control bit is true
            bool  cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            block cw_seed        = key.cw_seed[current_level + 3];
            expanded_seeds[0]    = _mm_xor_si128(expanded_seeds[0], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][0]]));
            expanded_seeds[1]    = _mm_xor_si128(expanded_seeds[1], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][1]]));
            expanded_seeds[2]    = _mm_xor_si128(expanded_seeds[2], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][2]]));
            expanded_seeds[3]    = _mm_xor_si128(expanded_seeds[3], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][3]]));
            expanded_seeds[4]    = _mm_xor_si128(expanded_seeds[4], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][4]]));
            expanded_seeds[5]    = _mm_xor_si128(expanded_seeds[5], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][5]]));
            expanded_seeds[6]    = _mm_xor_si128(expanded_seeds[6], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][6]]));
            expanded_seeds[7]    = _mm_xor_si128(expanded_seeds[7], _mm_and_si128(cw_seed, zero_and_all_one[prev_control_bits[current_level][7]]));

            for (uint32_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint32_t i = 0; i < 8; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        for (uint32_t i = 0; i < 8; ++i) {
            block    output = _mm_xor_si128(prev_seeds[current_level][i], _mm_and_si128(zero_and_all_one[prev_control_bits[current_level][i]], key.output));
            uint64_t low    = _mm_cvtsi128_si64(output);
            uint64_t high   = _mm_extract_epi64(output, 1);

            uint32_t idx = i * last_idx + current_idx;
            for (int j = 0; j < 128; ++j) {
                size_t   db_idx = (idx * 128 + j) ^ pr;
                uint32_t bit    = static_cast<uint32_t>(
                    ((j < 64 ? (low >> j) : (high >> (j - 64))) & 1));
                uint32_t mask = 0u - bit;

                // XOR in only the selected database entry
                dp ^= (database[db_idx] & mask);
            }
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + std::to_string(dp));
#endif
    return dp;
}

void OblivSelectEvaluator::EvaluateNextSeed(
    const uint32_t current_level, const block &current_seed, const bool &current_control_bit,
    std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
    const fss::dpf::DpfKey &key) const {
    // Expand the seed and control bits
    G_.DoubleExpand(current_seed, expanded_seeds);
    expanded_control_bits[fss::kLeft]  = getLSB(expanded_seeds[fss::kLeft]);
    expanded_control_bits[fss::kRight] = getLSB(expanded_seeds[fss::kRight]);

    // Apply correction word if control bit is true
    const block mask            = key.cw_seed[current_level] & zero_and_all_one[current_control_bit];
    expanded_seeds[fss::kLeft]  = _mm_xor_si128(expanded_seeds[fss::kLeft], mask);
    expanded_seeds[fss::kRight] = _mm_xor_si128(expanded_seeds[fss::kRight], mask);

    const bool control_mask_left  = key.cw_control_left[current_level] & current_control_bit;
    const bool control_mask_right = key.cw_control_right[current_level] & current_control_bit;
    expanded_control_bits[fss::kLeft] ^= control_mask_left;
    expanded_control_bits[fss::kRight] ^= control_mask_right;
}

}    // namespace wm
}    // namespace fsswm
