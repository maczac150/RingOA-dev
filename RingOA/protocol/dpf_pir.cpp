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
    : dpf_key(id, params.GetParameters()), shr_in(0),
      params_(params),
      serialized_size_(CalculateSerializedSize()) {
}

size_t DpfPirKey::CalculateSerializedSize() const {
    return dpf_key.GetSerializedSize() + sizeof(shr_in);
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
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&shr_in), reinterpret_cast<const uint8_t *>(&shr_in) + sizeof(shr_in));

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
    std::memcpy(&shr_in, buffer.data() + offset, sizeof(shr_in));
}

void DpfPirKey::PrintKey(const bool detailed) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    if (detailed) {
        Logger::DebugLog(LOC, Logger::StrWithSep("DpfPir Key"));
        dpf_key.PrintKey(true);
        Logger::DebugLog(LOC, "(shr_in=" + ToString(shr_in) + ")");
        Logger::DebugLog(LOC, kDash);
    } else {
        Logger::DebugLog(LOC, "DpfPir Key");
        dpf_key.PrintKey(false);
        Logger::DebugLog(LOC, "(shr_in=" + ToString(shr_in) + ")");
    }
#endif
}

DpfPirKeyGenerator::DpfPirKeyGenerator(
    const DpfPirParameters     &params,
    sharing::AdditiveSharing2P &ss)
    : params_(params),
      gen_(params.GetParameters()), ss_(ss) {
}

std::pair<DpfPirKey, DpfPirKey> DpfPirKeyGenerator::GenerateKeys() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate DpfPir Keys"));
#endif

    // Initialize the keys
    std::array<DpfPirKey, 2>        keys     = {DpfPirKey(0, params_), DpfPirKey(1, params_)};
    std::pair<DpfPirKey, DpfPirKey> key_pair = std::make_pair(std::move(keys[0]), std::move(keys[1]));

    // Generate random inputs for the keys
    // uint64_t r_in = ss_.GenerateRandomValue();
    uint64_t r_in = 0;

    // Generate DPF keys
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> dpf_keys = gen_.GenerateKeys(r_in, 1);

    // Set DPF keys for each party
    key_pair.first.dpf_key  = std::move(dpf_keys.first);
    key_pair.second.dpf_key = std::move(dpf_keys.second);

    // Generate shared random values
    std::pair<uint64_t, uint64_t> r_in_sh = ss_.Share(r_in);
    key_pair.first.shr_in                 = r_in_sh.first;
    key_pair.second.shr_in                = r_in_sh.second;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "r_in: " + ToString(r_in));
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

uint64_t DpfPirEvaluator::EvaluateSharedInput(osuCrypto::Channel          &chl,
                                              const DpfPirKey             &key,
                                              const std::vector<uint64_t> &database,
                                              const uint64_t               index) const {

    uint64_t party_id = key.dpf_key.party_id;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating ZeroTest protocol with shared inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " index: " + ToString(index));
#endif

    if (party_id == 0) {
        // Reconstruct masked inputs
        uint64_t masked_x_0, masked_x_1, masked_x;
        ss_.EvaluateAdd(index, key.shr_in, masked_x_0);
        ss_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_0: " + ToString(masked_x_0) + ", " + ToString(masked_x_1));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x));
#endif
        return EvaluateMaskedInput(key, database, masked_x);
    } else {
        // Reconstruct masked inputs
        uint64_t masked_x_0, masked_x_1, masked_x;
        ss_.EvaluateAdd(index, key.shr_in, masked_x_1);
        ss_.Reconst(party_id, chl, masked_x_0, masked_x_1, masked_x);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, party_str + " masked_x_1: " + ToString(masked_x_1));
        Logger::DebugLog(LOC, party_str + " masked_x: " + ToString(masked_x));
#endif
        return EvaluateMaskedInput(key, database, masked_x);
    }
}

uint64_t DpfPirEvaluator::EvaluateMaskedInput(const DpfPirKey &key, const std::vector<uint64_t> &database, const uint64_t index) const {
    uint64_t party_id = key.dpf_key.party_id;
    uint64_t nu       = params_.GetParameters().GetTerminateBitsize();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Evaluating ZeroTest protocol with masked inputs");
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = (party_id == 0) ? "[P0]" : "[P1]";
    Logger::DebugLog(LOC, party_str + " index: " + ToString(index));
#endif

    std::vector<block> uv(1U << nu);
    // uint64_t output = ComputeDotProductBlockSIMD(key.dpf_key, database);
    uint64_t output = EvaluateFullDomainThenDotProduct(key.dpf_key, database, uv);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, party_str + " output: " + ToString(output));
#endif

    return output;
}

block DpfPirEvaluator::ComputeDotProductBlockSIMD(const fss::dpf::DpfKey &key, std::vector<block> &database) const {
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

        auto input_iter0 = database.data() + ((0UL << current_level) + current_idx) * 128;
        auto input_iter1 = database.data() + ((1UL << current_level) + current_idx) * 128;
        auto input_iter2 = database.data() + ((2UL << current_level) + current_idx) * 128;
        auto input_iter3 = database.data() + ((3UL << current_level) + current_idx) * 128;
        auto input_iter4 = database.data() + ((4UL << current_level) + current_idx) * 128;
        auto input_iter5 = database.data() + ((5UL << current_level) + current_idx) * 128;
        auto input_iter6 = database.data() + ((6UL << current_level) + current_idx) * 128;
        auto input_iter7 = database.data() + ((7UL << current_level) + current_idx) * 128;

        // Calculate the dot product
        for (uint64_t j = 0; j < 128; ++j) {
            auto input0 = input_iter0[j] & zero_and_all_one[seed_byte0[j]];
            auto input1 = input_iter1[j] & zero_and_all_one[seed_byte1[j]];
            auto input2 = input_iter2[j] & zero_and_all_one[seed_byte2[j]];
            auto input3 = input_iter3[j] & zero_and_all_one[seed_byte3[j]];
            auto input4 = input_iter4[j] & zero_and_all_one[seed_byte4[j]];
            auto input5 = input_iter5[j] & zero_and_all_one[seed_byte5[j]];
            auto input6 = input_iter6[j] & zero_and_all_one[seed_byte6[j]];
            auto input7 = input_iter7[j] & zero_and_all_one[seed_byte7[j]];

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

uint64_t DpfPirEvaluator::EvaluateFullDomainThenDotProduct(const fss::dpf::DpfKey &key, const std::vector<uint64_t> &database, std::vector<block> &outputs) const {
    // Evaluate the FDE
    eval_.EvaluateFullDomain(key, outputs);
    uint64_t db_sum = 0;

    // Optimized bit extraction + mask-based accumulation
    size_t num_blocks = outputs.size();    // = num_shares / 128
    for (size_t i = 0; i < num_blocks; ++i) {
        // Load one 128-bit block into registers
        uint64_t low  = outputs[i].get<uint64_t>()[0];
        uint64_t high = outputs[i].get<uint64_t>()[1];

        // Process all 128 bits in this block
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask = 0ULL - ((low >> j) & 1ULL);
            db_sum += (database[i * 128 + j] & mask);
        }
        for (int j = 0; j < 64; ++j) {
            const uint64_t mask = 0ULL - ((high >> j) & 1ULL);
            db_sum += (database[i * 128 + 64 + j] & mask);
        }
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Dot product result: " + ToString(db_sum));
#endif
    return db_sum;
}

void DpfPirEvaluator::EvaluateNextSeed(
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
