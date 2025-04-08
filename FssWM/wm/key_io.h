#ifndef WM_KEY_IO_H_
#define WM_KEY_IO_H_

#include <string>

#include "FssWM/fss/dpf_key.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/wm/obliv_select.h"

namespace fsswm {
namespace wm {

// Set key type
enum class KeyType
{
    kDpfKey,
    kOblivSelectKey,
    kFssWMKey,
};

class KeyIo {
public:
    /**
     * @brief Default constructor for KeyIo.
     * @param binary_mode A flag to indicate whether to use binary mode. Defaults to true.
     */
    explicit KeyIo(bool binary_mode = true)
        : binary_mode_(binary_mode) {
    }

    /**
     * @brief Save the key to a file.
     * @tparam KeyType The type of key to save.
     * @param file_path The file path to save the key.
     * @param key The key to save.
     */
    template <typename KeyType>
    void SaveKey(const std::string &file_path, const KeyType &key) {
        std::vector<uint8_t> buffer;
        key.Serialize(buffer);
        try {
            if (binary_mode_) {
                FileIo io(".key.bin");
                io.WriteToFileBinary(file_path, buffer);
            } else {
                FileIo io(".key");
                io.WriteToFile(file_path, buffer);
            }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Key saved successfully to " + file_path);
#endif
        } catch (const std::exception &e) {
            // throw std::runtime_error("Failed to save key: " + std::string(e.what()));
            Logger::FatalLog(LOC, "Failed to save key: " + std::string(e.what()));
            std::exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief Load the key from a file.
     * @tparam KeyType The type of key to load.
     * @param file_path The file path to load the key.
     * @param key The key to load.
     */
    template <typename KeyType>
    void LoadKey(const std::string &file_path, KeyType &key) {
        std::vector<uint8_t> buffer;
        try {
            if (binary_mode_) {
                FileIo io(".key.bin");
                io.ReadFromFileBinary(file_path, buffer);
            } else {
                FileIo io(".key");
                io.ReadFromFile(file_path, buffer);
            }
            if (buffer.empty()) {
                throw std::runtime_error("Loaded buffer is empty.");
            }
            key.Deserialize(buffer);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Key loaded successfully from " + file_path);
#endif
        } catch (const std::exception &e) {
            // throw std::runtime_error("Failed to load key: " + std::string(e.what()));
            Logger::FatalLog(LOC, "Failed to load key: " + std::string(e.what()));
            std::exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief Get the binary mode flag.
     * @return The binary mode flag.
     */
    bool GetBinaryMode() const {
        return binary_mode_;
    }

private:
    bool binary_mode_; /**< Flag to indicate whether to use binary mode. */
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_KEY_IO_H_
