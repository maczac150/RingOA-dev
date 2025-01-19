#include "share3_types.h"

#include <cstring>

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace sharing {

void SharePair::DebugLog(const uint32_t party_id) const {
    Logger::DebugLog(LOC, "[P" + std::to_string(party_id) + "] (x_" + std::to_string(party_id) + ", x_" + std::to_string((party_id + 1) % 3) + ") = (" + std::to_string(data[0]) + ", " + std::to_string(data[1]) + ")");
}

void SharePair::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing SharePair");
#endif

    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(data.data()) + sizeof(data));
}

void SharePair::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing SharePair");
#endif
    std::memcpy(data.data(), buffer.data(), sizeof(data));
}

void SharesPair::DebugLog(const uint32_t party_id) const {
    Logger::DebugLog(LOC, "[P" + std::to_string(party_id) + "] (x_" + std::to_string(party_id) + ", x_" + std::to_string((party_id - 1) % 3) + ") = (" + ToString(data[0]) + ", " + ToString(data[1]) + ")");
}

void SharesPair::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing SharesPair");
#endif

    // Serialize the number of shares
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_shares), reinterpret_cast<const uint8_t *>(&num_shares) + sizeof(num_shares));

    // Serialize the share data
    for (const auto &vec : data) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(vec.data()), reinterpret_cast<const uint8_t *>(vec.data()) + sizeof(vec));
    }
}

void SharesPair::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing SharesPair");
#endif
    size_t offset = 0;

    // Deserialize the number of shares
    std::memcpy(&num_shares, buffer.data() + offset, sizeof(num_shares));
    offset += sizeof(num_shares);

    // Resize the share data vectors
    data[0].resize(num_shares);
    data[1].resize(num_shares);

    // Deserialize the share data
    for (auto &vec : data) {
        std::memcpy(vec.data(), buffer.data() + offset, sizeof(vec));
        offset += sizeof(vec);
    }
}

}    // namespace sharing
}    // namespace fsswm
