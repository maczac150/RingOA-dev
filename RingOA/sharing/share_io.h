#ifndef SHARING_SHARE_IO_H_
#define SHARING_SHARE_IO_H_

#include "RingOA/utils/logger.h"

namespace ringoa {
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
        std::string full_path = file_path + ".sh.bin";

        std::ofstream ofs(full_path, std::ios::binary | std::ios::out);
        if (!ofs.is_open()) {
            Logger::FatalLog(LOC, "Failed to open file for saving share: " + full_path);
            exit(EXIT_FAILURE);
        }
        try {
            share.SerializeToStream(ofs);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Saved share to file: " + full_path);
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
        std::string full_path = file_path + ".sh.bin";

        std::ifstream ifs(full_path, std::ios::binary | std::ios::in);
        if (!ifs.is_open()) {
            Logger::FatalLog(LOC, "Failed to open file for loading share: " + full_path);
            exit(EXIT_FAILURE);
        }
        try {
            share.DeserializeFromStream(ifs);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Loaded share from file: " + full_path);
#endif
        } catch (const std::exception &e) {
            Logger::FatalLog(LOC, "Error loading share from file: " + std::string(e.what()));
            exit(EXIT_FAILURE);
        }
    }
};

}    // namespace sharing
}    // namespace ringoa

#endif    // SHARING_SHARE_IO_H_
