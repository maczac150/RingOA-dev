#include "share3_types.h"

#include <cstring>

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace sharing {

void RepShare::DebugLog(const uint32_t party_id, const std::string &prefix) const {
    Logger::DebugLog(LOC, "[P" + std::to_string(party_id) + "] (" + prefix + "_" + std::to_string(party_id) + ", " + prefix + "_" + std::to_string((party_id + 1) % 3) + ") = (" + std::to_string(data[0]) + ", " + std::to_string(data[1]) + ")");
}

void RepShare::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing RepShare");
#endif

    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(data.data()), reinterpret_cast<const uint8_t *>(data.data()) + sizeof(data));
}

void RepShare::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing RepShare");
#endif
    std::memcpy(data.data(), buffer.data(), sizeof(data));
}

void RepShareVec::DebugLog(const uint32_t party_id, const std::string &prefix) const {
    Logger::DebugLog(LOC, "[P" + std::to_string(party_id) + "] (" + prefix + "_" + std::to_string(party_id) + ", " + prefix + "_" + std::to_string((party_id + 1) % 3) + ") = (" + ToString(data[0]) + ", " + ToString(data[1]) + ")");
}

void RepShareVec::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing RepShareVec");
#endif

    // Serialize the number of shares
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_shares), reinterpret_cast<const uint8_t *>(&num_shares) + sizeof(num_shares));

    // Serialize the share data
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(data[0].data()), reinterpret_cast<const uint8_t *>(data[0].data()) + num_shares * sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(data[1].data()), reinterpret_cast<const uint8_t *>(data[1].data()) + num_shares * sizeof(uint32_t));
}

void RepShareVec::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing RepShareVec");
#endif
    size_t offset = 0;

    // Deserialize the number of shares
    std::memcpy(&num_shares, buffer.data() + offset, sizeof(num_shares));
    offset += sizeof(num_shares);

    // Resize the share data vectors
    data[0].resize(num_shares);
    data[1].resize(num_shares);

    // Deserialize the share data
    std::memcpy(data[0].data(), buffer.data() + offset, num_shares * sizeof(uint32_t));
    offset += num_shares * sizeof(uint32_t);
    std::memcpy(data[1].data(), buffer.data() + offset, num_shares * sizeof(uint32_t));
}

void RepShareVecView::DebugLog(const uint32_t party_id, const std::string &prefix) const {
    Logger::DebugLog(LOC, "[P" + std::to_string(party_id) + "] (" + prefix + "_" + std::to_string(party_id) + ", " + prefix + "_" + std::to_string((party_id + 1) % 3) + ") = (" + ToString(share0) + ", " + ToString(share1) + ")");
}

void RepShareMat::DebugLog(const uint32_t party_id, const std::string &prefix) const {
    Logger::DebugLog(LOC, "[P" + std::to_string(party_id) + "] (" + prefix + "_" + std::to_string(party_id) + ", " + prefix + "_" + std::to_string((party_id + 1) % 3) + ") = (" + ToStringMat(data[0]) + ", " + ToStringMat(data[1]) + ")");
}

void RepShareMat::Serialize(std::vector<uint8_t> &buffer) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Serializing RepShareMat");
#endif

    // Serialize the number of rows and columns
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&rows), reinterpret_cast<const uint8_t *>(&rows) + sizeof(rows));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&cols), reinterpret_cast<const uint8_t *>(&cols) + sizeof(cols));

    // Serialize the share data
    for (size_t i = 0; i < rows; ++i) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(data[0][i].data()), reinterpret_cast<const uint8_t *>(data[0][i].data()) + cols * sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(data[1][i].data()), reinterpret_cast<const uint8_t *>(data[1][i].data()) + cols * sizeof(uint32_t));
    }
}

void RepShareMat::Deserialize(const std::vector<uint8_t> &buffer) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Deserializing RepShareMat");
#endif

    size_t offset = 0;

    // Deserialize the number of rows and columns
    std::memcpy(&rows, buffer.data() + offset, sizeof(rows));
    offset += sizeof(rows);
    std::memcpy(&cols, buffer.data() + offset, sizeof(cols));
    offset += sizeof(cols);

    // Resize the share data vectors
    data[0].resize(rows, std::vector<uint32_t>(cols));
    data[1].resize(rows, std::vector<uint32_t>(cols));

    // Deserialize the share data
    for (size_t i = 0; i < rows; ++i) {
        std::memcpy(data[0][i].data(), buffer.data() + offset, cols * sizeof(uint32_t));
        offset += cols * sizeof(uint32_t);
        std::memcpy(data[1][i].data(), buffer.data() + offset, cols * sizeof(uint32_t));
        offset += cols * sizeof(uint32_t);
    }
}

}    // namespace sharing
}    // namespace fsswm
