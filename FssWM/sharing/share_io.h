#ifndef SHARING_SHARE_IO_H_
#define SHARING_SHARE_IO_H_

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"

namespace fsswm {
namespace sharing {

class ShareIo {
public:
    /**
     * @brief Constructs a ShareIo object with a specified binary mode.
     */
    explicit ShareIo() = default;

    /**
     * @brief Saves a share to a file.
     * @tparam ShareType The type of share to be saved.
     * @param file_path The file path to save the share.
     * @param share The share to be saved.
     */
    template <typename ShareType>
    void SaveShare(const std::string &file_path, const ShareType &share) const {
        std::vector<uint8_t> buffer;
        share.Serialize(buffer);
        try {
            FileIo io(".sh.bin");
            io.WriteBinary(file_path, buffer);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Saved share to file: " + file_path + ".sh.bin");
#endif
        } catch (const std::exception &e) {
            Logger::FatalLog(LOC, "Error saving share to file: " + std::string(e.what()));
            exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief Loads a share from a file.
     * @tparam ShareType The type of share to be loaded.
     * @param file_path The file path to load the share.
     * @param share The share to be loaded.
     */
    template <typename ShareType>
    void LoadShare(const std::string &file_path, ShareType &share) const {
        std::vector<uint8_t> buffer;
        try {
            FileIo io(".sh.bin");
            io.ReadBinary(file_path, buffer);
            share.Deserialize(buffer);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Loaded share from file: " + file_path + ".sh.bin");
#endif
        } catch (const std::exception &e) {
            Logger::FatalLog(LOC, "Error loading share from file: " + std::string(e.what()));
            exit(EXIT_FAILURE);
        }
    }
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_SHARE_IO_H_
