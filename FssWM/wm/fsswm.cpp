#include "fsswm.h"

#include <cstring>

#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "plain_wm.h"

namespace fsswm {
namespace wm {

using fsswm::sharing::BinaryReplicatedSharing3P;

void FssWMParameters::PrintParameters() const {
    Logger::DebugLog(LOC, "[FssWM Parameters]" + GetParametersInfo());
}

FssWMKey::FssWMKey(const uint64_t id, const FssWMParameters &params)
    : num_os_keys(params.GetSigma()),
      params_(params) {
    os_keys.reserve(num_os_keys);
    for (uint64_t i = 0; i < num_os_keys; ++i) {
        os_keys.emplace_back(OblivSelectKey(id, params.GetOSParameters()));
    }
    serialized_size_ = CalculateSerializedSize();
}

size_t FssWMKey::CalculateSerializedSize() const {
    size_t size = sizeof(num_os_keys);
    for (uint64_t i = 0; i < num_os_keys; ++i) {
        size += os_keys[i].GetSerializedSize();
    }
    return size;
}

void FssWMKey::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing FssWMKey");
#endif

    // Serialize the number of OS keys
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_os_keys), reinterpret_cast<const uint8_t *>(&num_os_keys) + sizeof(num_os_keys));

    // Serialize the OS keys
    for (const auto &os_key : os_keys) {
        std::vector<uint8_t> key_buffer;
        os_key.Serialize(key_buffer);
        buffer.insert(buffer.end(), key_buffer.begin(), key_buffer.end());
    }

    // Check size
    if (buffer.size() != serialized_size_) {
        Logger::ErrorLog(LOC, "Serialized size mismatch: " + ToString(buffer.size()) + " != " + ToString(serialized_size_));
        return;
    }
}

void FssWMKey::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing FssWMKey");
#endif
    size_t offset = 0;

    // Deserialize the number of OS keys
    std::memcpy(&num_os_keys, buffer.data() + offset, sizeof(num_os_keys));
    offset += sizeof(num_os_keys);

    // Deserialize the OS keys
    for (auto &os_key : os_keys) {
        size_t key_size = os_key.GetSerializedSize();
        os_key.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.begin() + offset + key_size));
        offset += key_size;
    }
}

void FssWMKey::PrintKey(const bool detailed) const {
    Logger::DebugLog(LOC, Logger::StrWithSep("FssWM Key"));
    Logger::DebugLog(LOC, "Number of OblivSelect Keys: " + ToString(num_os_keys));
    for (uint64_t i = 0; i < num_os_keys; ++i) {
        os_keys[i].PrintKey(detailed);
    }
}

FssWMKeyGenerator::FssWMKeyGenerator(
    const FssWMParameters              &params,
    sharing::BinarySharing2P           &bss,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      os_gen_(params.GetOSParameters(), bss),
      brss_(brss) {
}

std::array<sharing::RepShareMatBlock, 3> FssWMKeyGenerator::GenerateDatabaseShare(const FMIndex &fm) {
    if (fm.GetWaveletMatrix().GetLength() + 1 != params_.GetDatabaseSize()) {
        throw std::invalid_argument("FMIndex length does not match the database size in FssWMParameters");
    }
    const std::vector<uint64_t> &rank0_tables = fm.GetRank0Tables();
    const std::vector<uint64_t> &rank1_tables = fm.GetRank1Tables();
    std::vector<block>           database(rank0_tables.size());
    for (size_t i = 0; i < rank0_tables.size(); ++i) {
        database[i] = osuCrypto::toBlock(rank1_tables[i], rank0_tables[i]);
    }
    return brss_.ShareLocal(database, rank0_tables.size(), fm.GetWaveletMatrix().GetSigma());
}

std::array<FssWMKey, 3> FssWMKeyGenerator::GenerateKeys() const {
    // Initialize the keys
    std::array<FssWMKey, 3> keys = {
        FssWMKey(0, params_),
        FssWMKey(1, params_),
        FssWMKey(2, params_),
    };

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Generate FssWM keys"));
#endif

    for (uint64_t i = 0; i < keys[0].num_os_keys; ++i) {
        // Generate the OblivSelect keys
        std::array<OblivSelectKey, 3> os_key = os_gen_.GenerateKeys();

        // Set the OblivSelect keys
        keys[0].os_keys[i] = std::move(os_key[0]);
        keys[1].os_keys[i] = std::move(os_key[1]);
        keys[2].os_keys[i] = std::move(os_key[2]);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "FssWM keys generated");
    keys[0].PrintKey();
    keys[1].PrintKey();
    keys[2].PrintKey();
#endif

    // Return the keys
    return keys;
}

FssWMEvaluator::FssWMEvaluator(
    const FssWMParameters              &params,
    sharing::BinaryReplicatedSharing3P &brss)
    : params_(params),
      os_eval_(params.GetOSParameters(), brss),
      brss_(brss) {
}

void FssWMEvaluator::EvaluateRankCF(Channels                        &chls,
                                    const FssWMKey                  &key,
                                    const sharing::RepShareMatBlock &wm_tables,
                                    const sharing::RepShareView64   &char_sh,
                                    sharing::RepShare64             &position_sh,
                                    sharing::RepShare64             &result) const {

    uint64_t d        = params_.GetDatabaseBitSize();
    uint64_t ds       = params_.GetDatabaseSize();
    uint64_t sigma    = params_.GetSigma();
    uint64_t party_id = chls.party_id;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, Logger::StrWithSep("Evaluate FssWM key"));
    Logger::DebugLog(LOC, "Database bit size: " + ToString(d));
    Logger::DebugLog(LOC, "Database size: " + ToString(ds));
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma));
    Logger::DebugLog(LOC, "Party ID: " + ToString(party_id));
    std::string party_str = "[P" + ToString(party_id) + "] ";
#endif

    sharing::RepShareBlock rank01_sh;
    sharing::RepShare64    rank0_sh(0, 0), rank1_sh(0, 0);
    for (uint64_t i = 0; i < sigma; ++i) {
        os_eval_.Evaluate(chls, key.os_keys[i], wm_tables.RowView(i), position_sh, rank01_sh);
        // TODO: rank01_shをrank0_shとrank1_shに分離する
        rank0_sh[0] = rank01_sh[0].get<uint64_t>()[0];
        rank1_sh[0] = rank01_sh[0].get<uint64_t>()[1];
        rank0_sh[1] = rank01_sh[1].get<uint64_t>()[0];
        rank1_sh[1] = rank01_sh[1].get<uint64_t>()[1];
        brss_.EvaluateSelect(chls, rank0_sh, rank1_sh, char_sh.At(i), position_sh);
    }
    result = position_sh;
}

}    // namespace wm
}    // namespace fsswm
