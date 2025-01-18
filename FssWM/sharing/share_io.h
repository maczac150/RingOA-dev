#ifndef SHARING_SHARE_IO_H_
#define SHARING_SHARE_IO_H_

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "share3_types.h"

namespace fsswm {
namespace sharing {

enum class ShareType
{
    kSharePair,
    kSharesPair
};

class ShareIo {
public:
    /**
     * @brief Constructs a ShareIo object with a specified binary mode.
     * @param binary_mode If true, uses binary mode for serialization. Defaults to true.
     */
    explicit ShareIo(const bool binary_mode = true)
        : binary_mode_(binary_mode) {
    }

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
            std::string ext = binary_mode_ ? ".sh.bin" : ".sh.dat";
            utils::FileIo io(ext);
            if (binary_mode_) {
                io.WriteToFileBinary(file_path, buffer);
            } else {
                utils::FileIo io(".sh.dat");
            }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            utils::Logger::DebugLog(LOC, "Saved share to file: " + file_path + ext);
#endif
        } catch (const std::exception &e) {
            utils::Logger::FatalLog(LOC, "Error saving share to file: " + std::string(e.what()));
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
            if (binary_mode_) {
                utils::FileIo io(".sh.bin");
                io.ReadFromFileBinary(file_path, buffer);
            } else {
                utils::FileIo io(".sh.dat");
                io.ReadFromFile(file_path, buffer);
            }
            share.Deserialize(buffer);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            utils::Logger::DebugLog(LOC, "Loaded share from file: " + file_path);
#endif
        } catch (const std::exception &e) {
            utils::Logger::FatalLog(LOC, "Error loading share from file: " + std::string(e.what()));
            exit(EXIT_FAILURE);
        }
    }

private:
    bool binary_mode_; /**< Whether to use binary mode for serialization */
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_SHARE_IO_H_
