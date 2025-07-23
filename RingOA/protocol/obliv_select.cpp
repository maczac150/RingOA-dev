#include "obliv_select.h"

#include <cstring>

#include "RingOA/fss/prg.h"
#include "RingOA/sharing/binary_2p.h"
#include "RingOA/sharing/binary_3p.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace ringoa {
namespace proto {

void OblivSelectParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[Obliv Select Parameters]" + GetParametersInfo());
}

OblivSelectKey::OblivSelectKey(const uint64_t id, const OblivSelectParameters &params)
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
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
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
        Logger::DebugLog(LOC, Logger::StrWithSep("OblivSelect Key [Party " + ToString(party_id) + "]"));
        prev_key.PrintKey(true);
        next_key.PrintKey(true);
        Logger::DebugLog(LOC, "(prev_r_sh, next_r_sh): (" + ToString(prev_r_sh) + ", " + ToString(next_r_sh) + ")");
        Logger::DebugLog(LOC, "(r, r_sh_0, r_sh_1): (" + ToString(r) + ", " + ToString(r_sh_0) + ", " + ToString(r_sh_1) + ")");
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "OblivSelect Key [Party " + ToString(party_id) + "]");
        prev_key.PrintKey(false);
        next_key.PrintKey(false);
        Logger::DebugLog(LOC, "(prev_r_sh, next_r_sh): (" + ToString(prev_r_sh) + ", " + ToString(next_r_sh) + ")");
        Logger::DebugLog(LOC, "(r, r_sh_0, r_sh_1): (" + ToString(r) + ", " + ToString(r_sh_0) + ", " + ToString(r_sh_1) + ")");
    }
#endif
}

OblivSelectKeyGenerator::OblivSelectKeyGenerator(
    const OblivSelectParameters &params,
    sharing::BinarySharing2P    &bss)
    : params_(params),
      gen_(params.GetParameters()), bss_(bss) {
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

    std::array<uint64_t, 3>                                    rands;
    std::array<std::pair<uint64_t, uint64_t>, 3>               rand_shs;
    std::vector<std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey>> key_pairs;
    key_pairs.reserve(3);

    for (uint64_t i = 0; i < 3; ++i) {
        rands[i]    = bss_.GenerateRandomValue();
        rand_shs[i] = bss_.Share(rands[i]);
        key_pairs.push_back(gen_.GenerateKeys(rands[i], 1));

        keys[i].r      = rands[i];
        keys[i].r_sh_0 = rand_shs[i].first;
        keys[i].r_sh_1 = rand_shs[i].second;
    }

    // Assign previous and next keys
    for (uint64_t i = 0; i < 3; ++i) {
        uint64_t prev     = (i + 2) % 3;
        uint64_t next     = (i + 1) % 3;
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
    return keys;
}

OblivSelectEvaluator::OblivSelectEvaluator(
    const OblivSelectParameters        &params,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params), eval_(params.GetParameters()), brss_(brss),
      G_(fss::prg::PseudoRandomGenerator::GetInstance()) {
}

void OblivSelectEvaluator::Evaluate(Channels                         &chls,
                                    const OblivSelectKey             &key,
                                    const sharing::RepShareViewBlock &database,
                                    const sharing::RepShare64        &index,
                                    sharing::RepShareBlock           &result) const {

    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
    Logger::DebugLog(LOC, party_str + " idx: " + index.ToString());
    Logger::DebugLog(LOC, party_str + " db: " + database.ToString());
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructPRBinary(chls, key, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + ToString(pr_prev) + ", pr_next: " + ToString(pr_next));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block == __m128i)
    block dp_prev = ComputeDotProductBlockSIMD(key.prev_key, database.share0, pr_prev);
    block dp_next = ComputeDotProductBlockSIMD(key.next_key, database.share1, pr_next);

    block                  selected_sh = dp_prev ^ dp_next;
    sharing::RepShareBlock r_sh;
    brss_.Rand(r_sh);
    result[0] = selected_sh ^ r_sh[0] ^ r_sh[1];
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
}

void OblivSelectEvaluator::Evaluate(Channels                      &chls,
                                    const OblivSelectKey          &key,
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
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate OblivSelect key"));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
    Logger::DebugLog(LOC, party_str + " idx: " + index.ToString());
    Logger::DebugLog(LOC, party_str + " db: " + database.ToString());
#endif

    // Reconstruct p ^ r_i
    auto [pr_prev, pr_next] = ReconstructPRBinary(chls, key, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev: " + ToString(pr_prev) + ", pr_next: " + ToString(pr_next));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block
    auto [dp_prev, dp_next] = EvaluateFullDomainThenDotProduct(
        key.prev_key, key.next_key, uv_prev, uv_next, database, pr_prev, pr_next);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + "dp_prev: " + ToString(dp_prev) + ", dp_next: " + ToString(dp_next));
#endif

    uint64_t            selected_sh = dp_prev ^ dp_next;
    sharing::RepShare64 r_sh;
    brss_.Rand(r_sh);
    result[0] = selected_sh ^ r_sh[0] ^ r_sh[1];
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " result: " + ToString(result[0]) + ", " + ToString(result[1]));
#endif
}

void OblivSelectEvaluator::Evaluate_Parallel(Channels                      &chls,
                                             const OblivSelectKey          &key1,
                                             const OblivSelectKey          &key2,
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
    std::array<uint64_t, 4> pr = ReconstructPRBinary(chls, key1, key2, index);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " pr_prev1: " + ToString(pr[0]) + ", pr_next1: " + ToString(pr[1]) +
                              ", pr_prev2: " + ToString(pr[2]) + ", pr_next2: " + ToString(pr[3]));
#endif

    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block
    auto [dp_prev1, dp_next1] = EvaluateFullDomainThenDotProduct(
        key1.prev_key, key1.next_key, uv_prev, uv_next, database, pr[0], pr[1]);
    auto [dp_prev2, dp_next2] = EvaluateFullDomainThenDotProduct(
        key2.prev_key, key2.next_key, uv_prev, uv_next, database, pr[2], pr[3]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + "dp_prev1: " + ToString(dp_prev1) + ", dp_next1: " + ToString(dp_next1));
    Logger::DebugLog(LOC, party_str + "dp_prev2: " + ToString(dp_prev2) + ", dp_next2: " + ToString(dp_next2));
#endif

    uint64_t            selected1_sh = dp_prev1 ^ dp_next1;
    uint64_t            selected2_sh = dp_prev2 ^ dp_next2;
    sharing::RepShare64 r1_sh, r2_sh;
    brss_.Rand(r1_sh);
    brss_.Rand(r2_sh);
    result[0][0] = selected1_sh ^ r1_sh[0] ^ r1_sh[1];
    result[0][1] = selected2_sh ^ r2_sh[0] ^ r2_sh[1];
    chls.next.send(result[0]);
    chls.prev.recv(result[1]);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " result: " + ToString(result[0]) + ", " + ToString(result[1]));
#endif
}

block OblivSelectEvaluator::ComputeDotProductBlockSIMD(const fss::dpf::DpfKey       &key,
                                                       const std::span<const block> &database,
                                                       const uint64_t                pr) const {
    uint64_t nu = params_.GetParameters().GetTerminateBitsize();

    // Breadth-first traversal for 8 nodes
    std::vector<block> start_seeds{key.init_seed}, next_seeds;
    std::vector<bool>  start_control_bits{key.party_id != 0}, next_control_bits;

    for (uint64_t i = 0; i < 3; ++i) {
        std::array<block, 2> expanded_seeds;
        std::array<bool, 2>  expanded_control_bits;
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
    uint64_t current_level = 0;
    uint64_t current_idx   = 0;
    uint64_t last_depth    = std::max(static_cast<int32_t>(nu) - 3, 0);
    uint64_t last_idx      = 1U << last_depth;

    // Store the seeds and control bits
    std::array<block, 8>              expanded_seeds, output_seeds;
    std::array<block, 8>              sums = {zero_block, zero_block, zero_block, zero_block,
                                              zero_block, zero_block, zero_block, zero_block};
    std::array<bool, 8>               expanded_control_bits;
    std::vector<std::array<block, 8>> prev_seeds(last_depth + 1);
    std::vector<std::array<bool, 8>>  prev_control_bits(last_depth + 1);
    std::vector<block>                byte_expanded_seeds(64);

    // Evaluate the DPF key
    for (uint64_t i = 0; i < 8; ++i) {
        prev_seeds[0][i]        = start_seeds[i];
        prev_control_bits[0][i] = start_control_bits[i];
    }

    while (current_idx < last_idx) {
        while (current_level < last_depth) {
            // Expand the seed and control bits
            uint64_t mask        = (current_idx >> (last_depth - 1U - current_level));
            bool     current_bit = mask & 1U;

            G_.Expand(prev_seeds[current_level], expanded_seeds, current_bit);
            for (uint64_t i = 0; i < 8; ++i) {
                expanded_control_bits[i] = GetLsb(expanded_seeds[i]);
                SetLsbZero(expanded_seeds[i]);
            }

            // Apply correction word if control bit is true
            bool  cw_control_bit = current_bit ? key.cw_control_right[current_level + 3] : key.cw_control_left[current_level + 3];
            block cw_seed        = key.cw_seed[current_level + 3];
            expanded_seeds[0] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][0]]);
            expanded_seeds[1] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][1]]);
            expanded_seeds[2] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][2]]);
            expanded_seeds[3] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][3]]);
            expanded_seeds[4] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][4]]);
            expanded_seeds[5] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][5]]);
            expanded_seeds[6] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][6]]);
            expanded_seeds[7] ^= (cw_seed & zero_and_all_one[prev_control_bits[current_level][7]]);

            for (uint64_t i = 0; i < 8; ++i) {
                // expanded_seeds[i] ^= (key.cw_seed[current_level + 3] & zero_and_all_one[prev_control_bits[current_level][i]]);
                expanded_control_bits[i] ^= (cw_control_bit & prev_control_bits[current_level][i]);
            }

            // Update the current level
            current_level++;

            // Update the previous seeds and control bits
            for (uint64_t i = 0; i < 8; ++i) {
                prev_seeds[current_level][i]        = expanded_seeds[i];
                prev_control_bits[current_level][i] = expanded_control_bits[i];
            }
        }

        // Seed expansion for the final output
        G_.Expand(prev_seeds[current_level], prev_seeds[current_level], true);

        for (uint64_t j = 0; j < 8; ++j) {
            output_seeds[j] = prev_seeds[current_level][j] ^ (zero_and_all_one[prev_control_bits[current_level][j]] & key.output);
        }

        auto dest = byte_expanded_seeds.data();

        for (uint64_t i = 0; i < 8; ++i) {
            dest[0] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(0);
            dest[1] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(1);
            dest[2] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(2);
            dest[3] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(3);
            dest[4] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(4);
            dest[5] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(5);
            dest[6] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(6);
            dest[7] = all_bytes_one_mask & output_seeds[i].mm_srai_epi16(7);

            dest += 8;
        }

        uint8_t *seed_byte0 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 0;
        uint8_t *seed_byte1 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 1;
        uint8_t *seed_byte2 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 2;
        uint8_t *seed_byte3 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 3;
        uint8_t *seed_byte4 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 4;
        uint8_t *seed_byte5 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 5;
        uint8_t *seed_byte6 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 6;
        uint8_t *seed_byte7 = reinterpret_cast<uint8_t *>(byte_expanded_seeds.data()) + 128 * 7;

        // Calculate the dot product
        for (uint64_t j = 0; j < 128; ++j) {
            size_t db_idx0 = (((0 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx1 = (((1 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx2 = (((2 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx3 = (((3 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx4 = (((4 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx5 = (((5 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx6 = (((6 * last_idx + current_idx) * 128) + j) ^ pr;
            size_t db_idx7 = (((7 * last_idx + current_idx) * 128) + j) ^ pr;
            auto   input0  = database[db_idx0] & zero_and_all_one[seed_byte0[j]];
            auto   input1  = database[db_idx1] & zero_and_all_one[seed_byte1[j]];
            auto   input2  = database[db_idx2] & zero_and_all_one[seed_byte2[j]];
            auto   input3  = database[db_idx3] & zero_and_all_one[seed_byte3[j]];
            auto   input4  = database[db_idx4] & zero_and_all_one[seed_byte4[j]];
            auto   input5  = database[db_idx5] & zero_and_all_one[seed_byte5[j]];
            auto   input6  = database[db_idx6] & zero_and_all_one[seed_byte6[j]];
            auto   input7  = database[db_idx7] & zero_and_all_one[seed_byte7[j]];

            sums[0] = sums[0] ^ input0;
            sums[1] = sums[1] ^ input1;
            sums[2] = sums[2] ^ input2;
            sums[3] = sums[3] ^ input3;
            sums[4] = sums[4] ^ input4;
            sums[5] = sums[5] ^ input5;
            sums[6] = sums[6] ^ input6;
            sums[7] = sums[7] ^ input7;
        }

        // Update the current index
        int shift = (current_idx + 1U) ^ current_idx;
        current_level -= log2floor(shift) + 1;
        current_idx++;
    }

    block blk_sum = zero_block;
    for (uint64_t i = 0; i < 8; i++) {
        blk_sum = blk_sum ^ sums[i];
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + Format(blk_sum));
#endif
    return blk_sum;
}

std::pair<uint64_t, uint64_t> OblivSelectEvaluator::EvaluateFullDomainThenDotProduct(const fss::dpf::DpfKey        &key_prev,
                                                                                     const fss::dpf::DpfKey        &key_next,
                                                                                     std::vector<block>            &uv_prev,
                                                                                     std::vector<block>            &uv_next,
                                                                                     const sharing::RepShareView64 &database,
                                                                                     const uint64_t                 pr_prev,
                                                                                     const uint64_t                 pr_next) const {
    // Evaluate DPF (uv_prev and uv_next are std::vector<block>, where block == __m128i)
    eval_.EvaluateFullDomain(key_prev, uv_prev);
    eval_.EvaluateFullDomain(key_next, uv_next);
    uint64_t dp_prev = 0, dp_next = 0;

    for (size_t i = 0; i < uv_prev.size(); ++i) {
        const uint64_t low_prev  = uv_prev[i].get<uint64_t>()[0];
        const uint64_t high_prev = uv_prev[i].get<uint64_t>()[1];
        const uint64_t low_next  = uv_next[i].get<uint64_t>()[0];
        const uint64_t high_next = uv_next[i].get<uint64_t>()[1];

        for (int j = 0; j < 64; ++j) {
            const uint64_t mask_prev = 0ULL - ((low_prev >> j) & 1ULL);
            const uint64_t mask_next = 0ULL - ((low_next >> j) & 1ULL);
            dp_prev ^= (database.share0[(i * 128 + j) ^ pr_prev] & mask_prev);
            dp_next ^= (database.share1[(i * 128 + j) ^ pr_next] & mask_next);
        }
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask_prev = 0ULL - ((high_prev >> j) & 1ULL);
            const uint64_t mask_next = 0ULL - ((high_next >> j) & 1ULL);
            dp_prev ^= (database.share0[(i * 128 + 64 + j) ^ pr_prev] & mask_prev);
            dp_next ^= (database.share1[(i * 128 + 64 + j) ^ pr_next] & mask_next);
        }
    }
    return std::make_pair(dp_prev, dp_next);
}

std::pair<uint64_t, uint64_t> OblivSelectEvaluator::ReconstructPRBinary(Channels                  &chls,
                                                                        const OblivSelectKey      &key,
                                                                        const sharing::RepShare64 &index) const {

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "ReconstructPR for Party " + ToString(chls.party_id));
#endif

    // Set replicated sharing of random value
    uint64_t            pr_prev = 0, pr_next = 0;
    sharing::RepShare64 r_0_sh, r_1_sh, r_2_sh;

    switch (chls.party_id) {
        case 0:
            r_0_sh = sharing::RepShare64(key.r_sh_0, key.r_sh_1);
            r_1_sh = sharing::RepShare64(key.next_r_sh, 0);
            r_2_sh = sharing::RepShare64(0, key.prev_r_sh);
            break;
        case 1:
            r_0_sh = sharing::RepShare64(0, key.prev_r_sh);
            r_1_sh = sharing::RepShare64(key.r_sh_0, key.r_sh_1);
            r_2_sh = sharing::RepShare64(key.next_r_sh, 0);
            break;
        case 2:
            r_0_sh = sharing::RepShare64(key.next_r_sh, 0);
            r_1_sh = sharing::RepShare64(0, key.prev_r_sh);
            r_2_sh = sharing::RepShare64(key.r_sh_0, key.r_sh_1);
            break;
        default:
            Logger::ErrorLog(LOC, "Invalid party_id: " + ToString(chls.party_id));
            return std::make_pair(0, 0);
    }

    // Reconstruct p ^ r_i
    sharing::RepShare64 pr_prev_sh;
    sharing::RepShare64 pr_next_sh;

    if (chls.party_id == 0) {
        // p ^ r_1 between Party 0 and Party 2
        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_1_sh, pr_prev_sh);
        brss_.EvaluateXor(index, r_2_sh, pr_next_sh);
        chls.prev.send(pr_prev_sh[0]);
        chls.next.send(pr_next_sh[1]);
        uint64_t p_r_1_prev;
        uint64_t p_r_2_next;
        chls.next.recv(p_r_2_next);
        chls.prev.recv(p_r_1_prev);
        pr_next = p_r_1_prev ^ pr_prev_sh[0] ^ pr_prev_sh[1];
        pr_prev = pr_next_sh[0] ^ pr_next_sh[1] ^ p_r_2_next;

    } else if (chls.party_id == 1) {
        // p ^ r_0 between Party 1 and Party 2
        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_0_sh, pr_next_sh);
        brss_.EvaluateXor(index, r_2_sh, pr_prev_sh);
        chls.next.send(pr_next_sh[1]);
        chls.prev.send(pr_prev_sh[0]);
        uint64_t p_r_0_next;
        uint64_t p_r_2_prev;
        chls.prev.recv(p_r_2_prev);
        chls.next.recv(p_r_0_next);
        pr_next = p_r_2_prev ^ pr_prev_sh[0] ^ pr_prev_sh[1];
        pr_prev = pr_next_sh[0] ^ pr_next_sh[1] ^ p_r_0_next;

    } else {
        // p ^ r_0 between Party 1 and Party 2
        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(index, r_0_sh, pr_prev_sh);
        brss_.EvaluateXor(index, r_1_sh, pr_next_sh);
        chls.prev.send(pr_prev_sh[0]);
        chls.next.send(pr_next_sh[1]);
        uint64_t p_r_0_prev;
        uint64_t p_r_1_next;
        chls.prev.recv(p_r_0_prev);
        chls.next.recv(p_r_1_next);
        pr_next = p_r_0_prev ^ pr_prev_sh[0] ^ pr_prev_sh[1];
        pr_prev = pr_next_sh[0] ^ pr_next_sh[1] ^ p_r_1_next;
    }
    return std::make_pair(pr_prev, pr_next);
}

std::array<uint64_t, 4> OblivSelectEvaluator::ReconstructPRBinary(Channels                     &chls,
                                                                  const OblivSelectKey         &key1,
                                                                  const OblivSelectKey         &key2,
                                                                  const sharing::RepShareVec64 &index) const {

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "ReconstructPR for Party " + ToString(chls.party_id));
#endif

    // Set replicated sharing of random value
    uint64_t               pr_prev1 = 0, pr_next1 = 0, pr_prev2 = 0, pr_next2 = 0;
    sharing::RepShareVec64 r_0_sh(2), r_1_sh(2), r_2_sh(2);

    switch (chls.party_id) {
        case 0:
            r_0_sh.Set(0, sharing::RepShare64(key1.r_sh_0, key1.r_sh_1));
            r_1_sh.Set(0, sharing::RepShare64(key1.next_r_sh, 0));
            r_2_sh.Set(0, sharing::RepShare64(0, key1.prev_r_sh));
            r_0_sh.Set(1, sharing::RepShare64(key2.r_sh_0, key2.r_sh_1));
            r_1_sh.Set(1, sharing::RepShare64(key2.next_r_sh, 0));
            r_2_sh.Set(1, sharing::RepShare64(0, key2.prev_r_sh));
            break;
        case 1:
            r_0_sh.Set(0, sharing::RepShare64(0, key1.prev_r_sh));
            r_1_sh.Set(0, sharing::RepShare64(key1.r_sh_0, key1.r_sh_1));
            r_2_sh.Set(0, sharing::RepShare64(key1.next_r_sh, 0));
            r_0_sh.Set(1, sharing::RepShare64(0, key2.prev_r_sh));
            r_1_sh.Set(1, sharing::RepShare64(key2.r_sh_0, key2.r_sh_1));
            r_2_sh.Set(1, sharing::RepShare64(key2.next_r_sh, 0));
            break;
        case 2:
            r_0_sh.Set(0, sharing::RepShare64(key1.next_r_sh, 0));
            r_1_sh.Set(0, sharing::RepShare64(0, key1.prev_r_sh));
            r_2_sh.Set(0, sharing::RepShare64(key1.r_sh_0, key1.r_sh_1));
            r_0_sh.Set(1, sharing::RepShare64(key2.next_r_sh, 0));
            r_1_sh.Set(1, sharing::RepShare64(0, key2.prev_r_sh));
            r_2_sh.Set(1, sharing::RepShare64(key2.r_sh_0, key2.r_sh_1));
            break;
        default:
            Logger::ErrorLog(LOC, "Invalid party_id: " + ToString(chls.party_id));
            return std::array<uint64_t, 4>{0, 0, 0, 0};
    }

    // Reconstruct p ^ r_i
    sharing::RepShareVec64 pr_prev_sh(2);
    sharing::RepShareVec64 pr_next_sh(2);

    if (chls.party_id == 0) {
        // p ^ r_1 between Party 0 and Party 2
        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_1_sh, pr_prev_sh);
        brss_.EvaluateXor(index, r_2_sh, pr_next_sh);
        chls.prev.send(pr_prev_sh[0]);
        chls.next.send(pr_next_sh[1]);
        std::vector<uint64_t> p_r_1_prev(2);
        std::vector<uint64_t> p_r_2_next(2);
        chls.next.recv(p_r_2_next);
        chls.prev.recv(p_r_1_prev);
        pr_next1 = p_r_1_prev[0] ^ pr_prev_sh[0][0] ^ pr_prev_sh[1][0];
        pr_prev1 = pr_next_sh[0][0] ^ pr_next_sh[1][0] ^ p_r_2_next[0];
        pr_next2 = p_r_1_prev[1] ^ pr_prev_sh[0][1] ^ pr_prev_sh[1][1];
        pr_prev2 = pr_next_sh[0][1] ^ pr_next_sh[1][1] ^ p_r_2_next[1];

    } else if (chls.party_id == 1) {
        // p ^ r_0 between Party 1 and Party 2
        // p ^ r_2 between Party 0 and Party 1
        brss_.EvaluateXor(index, r_0_sh, pr_next_sh);
        brss_.EvaluateXor(index, r_2_sh, pr_prev_sh);
        chls.next.send(pr_next_sh[1]);
        chls.prev.send(pr_prev_sh[0]);
        std::vector<uint64_t> p_r_0_next(2);
        std::vector<uint64_t> p_r_2_prev(2);
        chls.prev.recv(p_r_2_prev);
        chls.next.recv(p_r_0_next);
        pr_next1 = p_r_2_prev[0] ^ pr_prev_sh[0][0] ^ pr_prev_sh[1][0];
        pr_prev1 = pr_next_sh[0][0] ^ pr_next_sh[1][0] ^ p_r_0_next[0];
        pr_next2 = p_r_2_prev[1] ^ pr_prev_sh[0][1] ^ pr_prev_sh[1][1];
        pr_prev2 = pr_next_sh[0][1] ^ pr_next_sh[1][1] ^ p_r_0_next[1];

    } else {
        // p ^ r_0 between Party 1 and Party 2
        // p ^ r_1 between Party 0 and Party 2
        brss_.EvaluateXor(index, r_0_sh, pr_prev_sh);
        brss_.EvaluateXor(index, r_1_sh, pr_next_sh);
        chls.prev.send(pr_prev_sh[0]);
        chls.next.send(pr_next_sh[1]);
        std::vector<uint64_t> p_r_0_prev(2);
        std::vector<uint64_t> p_r_1_next(2);
        chls.prev.recv(p_r_0_prev);
        chls.next.recv(p_r_1_next);
        pr_next1 = p_r_0_prev[0] ^ pr_prev_sh[0][0] ^ pr_prev_sh[1][0];
        pr_prev1 = pr_next_sh[0][0] ^ pr_next_sh[1][0] ^ p_r_1_next[0];
        pr_next2 = p_r_0_prev[1] ^ pr_prev_sh[0][1] ^ pr_prev_sh[1][1];
        pr_prev2 = pr_next_sh[0][1] ^ pr_next_sh[1][1] ^ p_r_1_next[1];
    }
    return std::array<uint64_t, 4>{pr_prev1, pr_next1, pr_prev2, pr_next2};
}

void OblivSelectEvaluator::EvaluateNextSeed(
    const uint64_t current_level, const block &current_seed, const bool &current_control_bit,
    std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
    const fss::dpf::DpfKey &key) const {
    // Expand the seed and control bits
    G_.DoubleExpand(current_seed, expanded_seeds);
    expanded_control_bits[fss::kLeft]  = GetLsb(expanded_seeds[fss::kLeft]);
    expanded_control_bits[fss::kRight] = GetLsb(expanded_seeds[fss::kRight]);
    SetLsbZero(expanded_seeds[fss::kLeft]);
    SetLsbZero(expanded_seeds[fss::kRight]);

    // Apply correction word if control bit is true
    const block mask = key.cw_seed[current_level] & zero_and_all_one[current_control_bit];
    expanded_seeds[fss::kLeft] ^= mask;
    expanded_seeds[fss::kRight] ^= mask;

    const bool control_mask_left  = key.cw_control_left[current_level] & current_control_bit;
    const bool control_mask_right = key.cw_control_right[current_level] & current_control_bit;
    expanded_control_bits[fss::kLeft] ^= control_mask_left;
    expanded_control_bits[fss::kRight] ^= control_mask_right;
}

}    // namespace proto
}    // namespace ringoa
