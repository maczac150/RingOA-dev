#ifndef UTILS_FILE_IO_H_
#define UTILS_FILE_IO_H_

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "logger.h"

namespace fsswm {

/**
 * @brief A class for file I/O operations.
 */
class FileIo {
public:
    /**
     * @brief Constructs a FileIo object with a specified file extension.
     * @param ext The default file extension (e.g., ".dat"). Defaults to ".dat".
     */
    explicit FileIo(const std::string ext = ".dat")
        : ext_(ext) {
    }

    /**
     * @brief Returns the file extension.
     * @return The file extension.
     */
    const std::string &GetExtension() const {
        return ext_;
    }

    /**
     * @brief Writes data to a file with a delimiter.
     * @tparam T The type of data to be written.
     * @param file_path The file name (without extension).
     * @param data The data to be written.
     * @param append If true, appends data to the file. Otherwise, overwrites it. Defaults to false.
     * @param delimiter The delimiter used to separate elements. Defaults to ','.
     */
    template <typename T>
    void WriteToFile(const std::string &file_path, const T &data, bool append = false, const std::string &delimiter = ",") {
        std::ofstream file;
        std::string   full_path = AddExtension(file_path);
        if (!OpenFileForWrite(file, full_path, append)) {
            Logger::FatalLog(LOC, "Failed to open file for writing: " + full_path);
            exit(EXIT_FAILURE);
        }

        if constexpr (std::is_same_v<T, std::string>) {
            // Single string
            file << 1 << "\n"
                 << data << "\n";
        } else if constexpr (std::is_arithmetic_v<T>) {
            // Single numeric
            file << 1 << "\n"
                 << data << "\n";
        }
        // For std::vector of strings
        else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            file << data.size() << "\n";
            for (size_t i = 0; i < data.size(); ++i) {
                file << data[i];
                if (i + 1 < data.size()) {
                    file << "\n";
                }
            }
            file << "\n";
        }
        // For std::vector of any arithmetic type
        else if constexpr (
            std::is_same_v<T, std::vector<typename T::value_type>> &&
            std::is_arithmetic_v<typename T::value_type>) {
            file << data.size() << "\n";
            for (size_t i = 0; i < data.size(); ++i) {
                file << data[i];
                if (i + 1 < data.size()) {
                    file << delimiter;
                }
            }
            file << "\n";
        }
        // For std::array (arithmetic or otherwise)
        else if constexpr (std::is_array_v<T> ||
                           std::is_same_v<T, std::array<typename T::value_type, std::tuple_size<T>::value>>) {
            constexpr size_t size = std::tuple_size<T>::value;
            file << size << "\n";
            for (size_t i = 0; i < size; ++i) {
                file << data[i];
                if (i + 1 < size) {
                    file << delimiter;
                }
            }
            file << "\n";
        } else {
            static_assert(sizeof(T) == 0, "Unsupported data type for writing.");
        }
        // File will be closed automatically upon leaving this scope
    }

    /**
     * @brief Reads data from a file using a delimiter.
     * @tparam T The type of data to be read.
     * @param file_path The file name (without extension).
     * @param data The variable to store the read data.
     * @param delimiter The delimiter used to separate elements. Defaults to ','.
     */
    template <typename T>
    void ReadFromFile(const std::string &file_path, T &data, const std::string &delimiter = ",") {
        std::ifstream file;
        std::string   full_path = AddExtension(file_path);
        if (!OpenFileForRead(file, full_path)) {
            Logger::FatalLog(LOC, "Failed to open file for reading: " + full_path);
            exit(EXIT_FAILURE);
        }

        std::string line;
        size_t      size = 0;

        // Read the number of elements (the first line)
        if (!std::getline(file, line)) {
            Logger::ErrorLog(LOC, "Failed to read the size from file: " + full_path);
            return;
        }
        try {
            size = std::stoul(line);
        } catch (...) {
            Logger::ErrorLog(LOC, "Invalid size format: " + line);
            return;
        }

        if constexpr (std::is_same_v<T, std::string>) {
            // Expecting a single string in the next line
            if (!std::getline(file, data)) {
                Logger::ErrorLog(LOC, "Failed to read string data: " + full_path);
            }
        } else if constexpr (std::is_arithmetic_v<T>) {
            // Expecting a single numeric value in the next space/tab-delimited read
            // (or next token). If you wrote it on one line with '<< data', just do:
            if (!(file >> data)) {
                Logger::ErrorLog(LOC, "Failed to read numeric data: " + full_path);
            }
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            // Reading vector of strings, size lines each containing a string
            // (if you wrote them each on a new line)
            std::vector<std::string> buffer(size);
            for (size_t i = 0; i < size; ++i) {
                if (!std::getline(file, buffer[i])) {
                    Logger::ErrorLog(LOC, "Failed to read string vector data: " + full_path);
                    return;
                }
            }
            data = std::move(buffer);
        }
        // For a vector of arithmetic types (e.g., int, float, uint8_t, etc.)
        else if constexpr (
            std::is_same_v<T, std::vector<typename T::value_type>> &&
            std::is_arithmetic_v<typename T::value_type>) {
            // The writing side puts all elements on one line (comma-delimited):
            // 1) The first line: size
            // 2) The second line: e.g. "10,20,30,40"
            std::vector<typename T::value_type> buffer(size);

            // Read the entire line containing the delimited data
            if (!std::getline(file, line)) {
                Logger::ErrorLog(LOC, "Failed to read numeric vector line: " + full_path);
                return;
            }

            std::stringstream ss(line);
            std::string       item;
            size_t            idx = 0;

            while (std::getline(ss, item, delimiter[0])) {
                if (idx >= size) {
                    Logger::ErrorLog(LOC, "More tokens than expected in file: " + full_path);
                    return;
                }
                // Convert the item to the appropriate arithmetic type
                // (Here we assume integral type; adjust as needed for floats)
                try {
                    if constexpr (std::is_floating_point_v<typename T::value_type>) {
                        buffer[idx] = static_cast<typename T::value_type>(std::stod(item));
                    } else {
                        buffer[idx] = static_cast<typename T::value_type>(std::stoll(item));
                    }
                } catch (...) {
                    Logger::ErrorLog(LOC, "Invalid numeric token: " + item);
                    return;
                }
                ++idx;
            }

            // Check if the number of tokens was less than size
            if (idx < size) {
                Logger::ErrorLog(LOC, "Fewer tokens than expected in file: " + full_path);
                return;
            }

            data = std::move(buffer);
        } else if constexpr (
            std::is_same_v<T, std::array<typename T::value_type, std::tuple_size<T>::value>>) {
            // Must match the exact array size
            constexpr size_t arraySize = std::tuple_size<T>::value;
            if (size != arraySize) {
                Logger::ErrorLog(LOC, "Data size mismatch for std::array: " + full_path);
                return;
            }

            // Read one line with all elements delimited
            if (!std::getline(file, line)) {
                Logger::ErrorLog(LOC, "Failed to read array data line: " + full_path);
                return;
            }

            std::array<typename T::value_type, arraySize> buffer;
            std::stringstream                             ss(line);
            std::string                                   item;
            size_t                                        idx = 0;

            while (std::getline(ss, item, delimiter[0])) {
                if (idx >= arraySize) {
                    Logger::ErrorLog(LOC, "More tokens than expected for std::array: " + full_path);
                    return;
                }
                try {
                    if constexpr (std::is_floating_point_v<typename T::value_type>) {
                        buffer[idx] = static_cast<typename T::value_type>(std::stod(item));
                    } else {
                        buffer[idx] = static_cast<typename T::value_type>(std::stoll(item));
                    }
                } catch (...) {
                    Logger::ErrorLog(LOC, "Invalid numeric token in array: " + item);
                    return;
                }
                ++idx;
            }

            if (idx < arraySize) {
                Logger::ErrorLog(LOC, "Fewer tokens than expected for std::array: " + full_path);
                return;
            }

            data = std::move(buffer);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported data type for reading.");
        }
    }

    /**
     * @brief Writes data to a file in binary mode.
     * @tparam T The type of data to be written.
     * @param file_path The file name (without extension).
     * @param data The data to be written.
     * @param append If true, appends data to the file. Otherwise, overwrites it. Defaults to false.
     */
    template <typename T>
    void WriteToFileBinary(const std::string &file_path, const T &data, bool append = false) {
        std::ios_base::openmode mode = std::ios::out | std::ios::binary;
        if (append) {
            mode |= std::ios::app;
        }

        std::string   full_path = AddExtension(file_path);
        std::ofstream file(full_path, mode);
        if (!file) {
            Logger::ErrorLog(LOC, "Failed to open file for binary writing: " + full_path);
            return;
        }

        try {
            // If T is a single arithmetic value
            if constexpr (std::is_arithmetic_v<T>) {
                // We store '1' as the number of elements
                uint32_t size = 1;
                file.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
                file.write(reinterpret_cast<const char *>(&data), sizeof(T));
            }
            // If T is a vector with an arithmetic value_type
            else if constexpr (
                std::is_same_v<T, std::vector<typename T::value_type>> &&
                std::is_arithmetic_v<typename T::value_type>) {
                uint32_t size = static_cast<uint32_t>(data.size());
                file.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
                file.write(reinterpret_cast<const char *>(data.data()), size * sizeof(typename T::value_type));
            }
            // If T is an std::array with an arithmetic value_type
            else if constexpr (
                std::is_same_v<T, std::array<typename T::value_type, std::tuple_size<T>::value>> &&
                std::is_arithmetic_v<typename T::value_type>) {
                constexpr size_t arrSize = std::tuple_size<T>::value;
                // same as above but arrSize is known at compile time
                uint32_t size = static_cast<uint32_t>(arrSize);
                file.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
                file.write(reinterpret_cast<const char *>(data.data()), arrSize * sizeof(typename T::value_type));
            } else {
                static_assert(sizeof(T) == 0, "Unsupported data type for binary writing.");
            }

            // Check for errors after write
            if (!file.good()) {
                throw std::runtime_error("I/O error occurred during binary writing.");
            }
        } catch (const std::exception &e) {
            Logger::ErrorLog(LOC, "Error during binary writing: " + std::string(e.what()));
        }
    }

    /**
     * @brief Reads data from a file in binary mode.
     * @tparam T The type of data to be read.
     * @param file_path The file name (without extension).
     * @param data The variable to store the read data.
     */
    template <typename T>
    void ReadFromFileBinary(const std::string &file_path, T &data) {
        std::ios_base::openmode mode = std::ios::in | std::ios::binary;

        // Open file for binary reading
        std::string   full_path = AddExtension(file_path);
        std::ifstream file(full_path, mode);
        if (!file) {
            Logger::FatalLog(LOC, "Failed to open file for binary reading: " + full_path);
            exit(EXIT_FAILURE);
        }

        try {
            // Read the number of elements (as uint32_t for portability)
            uint32_t size = 0;
            file.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));
            if (!file) {
                Logger::ErrorLog(LOC, "Failed to read size from file: " + full_path);
                return;
            }

            // Read binary data according to type
            if constexpr (std::is_arithmetic_v<T>) {
                // If the data is just a single arithmetic value, size must be 1
                if (size != 1ULL) {
                    Logger::ErrorLog(LOC, "Size mismatch for a single value: " + full_path);
                    return;
                }
                file.read(reinterpret_cast<char *>(&data), sizeof(T));
            }
            // For a vector whose element type is arithmetic (e.g. vector<int>, vector<float>, etc.)
            else if constexpr (
                std::is_same_v<T, std::vector<typename T::value_type>> &&
                std::is_arithmetic_v<typename T::value_type>) {
                // Resize the vector to hold 'size' elements
                data.resize(static_cast<size_t>(size));
                if (size > 0) {
                    file.read(reinterpret_cast<char *>(data.data()), size * sizeof(typename T::value_type));
                }
            }
            // For a std::array whose element type is arithmetic
            else if constexpr (
                std::is_same_v<T, std::array<typename T::value_type, std::tuple_size<T>::value>> &&
                std::is_arithmetic_v<typename T::value_type>) {
                constexpr size_t arrSize = std::tuple_size<T>::value;
                if (size != arrSize) {
                    Logger::ErrorLog(LOC, "Size mismatch for std::array type: " + full_path);
                    return;
                }
                file.read(reinterpret_cast<char *>(data.data()), arrSize * sizeof(typename T::value_type));
            } else {
                static_assert(sizeof(T) == 0, "Unsupported data type for binary reading.");
            }

            // Check stream state after reading
            if (!file) {
                Logger::ErrorLog(LOC, "Failed to read the required amount of data: " + full_path);
                return;
            }

        } catch (const std::exception &e) {
            Logger::ErrorLog(LOC, "Error during binary reading: " + std::string(e.what()));
        }
    }

    /**
     * @brief Clears the contents of a specified file.
     * @param file_path The file name (without extension).
     */
    void ClearFileContents(const std::string &file_path);

private:
    const std::string ext_; /**< Default file extension. */

    bool OpenFileForWrite(std::ofstream &file, const std::string &file_path, bool append);
    bool OpenFileForRead(std::ifstream &file, const std::string &file_path);

    /**
     * @brief Adds the default extension to a given file name.
     * @param file_path The file name without extension.
     * @return The full file name with extension.
     */
    std::string AddExtension(const std::string &file_path) const {
        return file_path + ext_;
    }
};

}    // namespace fsswm

#endif    // UTILS_FILE_IO_H_
