#ifndef PROTOCOL_KEY_IO_H_
#define PROTOCOL_KEY_IO_H_

#include <string>

#include "RingOA/fss/dpf_key.h"
#include "RingOA/utils/file_io.h"
#include "RingOA/utils/logger.h"

namespace ringoa {
namespace proto {

// Set key type
enum class KeyType
{
    kDpfKey,
    kDcfKey,
    kDdcfKey,
    kEqualityKey,
    kZeroTestKey,
    kIntegerComparisonKey,
    kOblivSelectKey,
    kRingOaKey,
    kOWMKey,
    kOFMIKey,
};

class KeyIo {
public:
    KeyIo() = default;

    template <typename KeyType>
    void SaveKey(const std::string &file_path, const KeyType &key) {
        std::vector<uint8_t> buffer;
        key.Serialize(buffer);
        FileIo io(".key.bin");
        io.WriteBinary(file_path, buffer);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Key saved successfully to " + file_path);
#endif
    }

    template <typename KeyType>
    void LoadKey(const std::string &file_path, KeyType &key) {
        std::vector<uint8_t> buffer;
        FileIo               io(".key.bin");
        io.ReadBinary(file_path, buffer);
        if (buffer.empty()) {
            throw std::runtime_error("Loaded buffer is empty: " + file_path);
        }
        key.Deserialize(buffer);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Key loaded successfully from " + file_path);
#endif
    }
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_KEY_IO_H_
