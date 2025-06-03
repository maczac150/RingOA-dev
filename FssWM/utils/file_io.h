#ifndef UTILS_FILE_IO_H_
#define UTILS_FILE_IO_H_

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "block.h"

namespace fsswm {

//=========================================================
// Type traits to detect which types are binary‐writable
//=========================================================
// 1) std::vector<arithmetic>
template <typename T>
struct is_vector_arith : std::false_type {};
template <typename U>
struct is_vector_arith<std::vector<U>> : std::bool_constant<std::is_arithmetic_v<U>> {};

// 2) std::vector<block>
template <typename T>
struct is_vector_block : std::false_type {};
template <>
struct is_vector_block<std::vector<block>> : std::true_type {};

// 3) std::array<arithmetic, N>
template <typename T>
struct is_array_arith : std::false_type {};
template <typename U, std::size_t N>
struct is_array_arith<std::array<U, N>> : std::bool_constant<std::is_arithmetic_v<U>> {};

// 4) std::array<block, N>
template <typename T>
struct is_array_block : std::false_type {};
template <std::size_t N>
struct is_array_block<std::array<block, N>> : std::true_type {};

/**
 * @brief A class for file I/O operations (binary/text).
 *        - Supports:
 *          ・Arithmetic types (int, float, etc.)
 *          ・block
 *          ・std::string
 *          ・std::vector<arithmetic> / std::vector<block>
 *          ・std::array<arithmetic, N> / std::array<block, N>
 *          ・std::vector<std::string> (In text format)
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
     * @brief Writes data to a file in binary mode.
     * @param T Enabled only if is_binary_writable<T>::value == true
     * @param file_path The file name (without extension).
     * @param data The data to be written.
     * @param append If true, appends data to the file. Otherwise, overwrites it. Defaults to false.
     */
    template <typename T>
    void WriteBinary(const std::string &file_path, const T &data, bool append = false) {
        // Compose full path with binary extension
        std::string full_path = AddExtension(file_path);

        // Open in binary mode
        std::ios_base::openmode mode = std::ios::out | std::ios::binary;
        if (append) {
            mode |= std::ios::app;
        }
        std::ofstream file(full_path, mode);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + full_path);
        }

        // Enable exceptions on write failures (badbit / failbit)
        file.exceptions(std::ios::badbit | std::ios::failbit);

        // If T is a single arithmetic value
        if constexpr (std::is_arithmetic_v<T>) {
            // We store '1' as the number of elements
            uint32_t count = 1;
            file.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));
            file.write(reinterpret_cast<const char *>(&data), sizeof(T));
        }
        // If T is a single block
        else if constexpr (std::is_same_v<T, block>) {
            uint32_t count = 1;
            file.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));
            file.write(reinterpret_cast<const char *>(&data), sizeof(block));
        }
        // If T is a std::string
        else if constexpr (std::is_same_v<T, std::string>) {
            uint32_t length = static_cast<uint32_t>(data.size());
            file.write(reinterpret_cast<const char *>(&length), sizeof(uint32_t));
            if (length > 0) {
                file.write(data.data(), length);
            }
        }
        // If T is a std::vector<arithmetic>
        else if constexpr (is_vector_arith<T>::value) {
            using ElemT    = typename T::value_type;
            uint32_t count = static_cast<uint32_t>(data.size());
            file.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));
            if (count > 0) {
                file.write(reinterpret_cast<const char *>(data.data()), count * sizeof(ElemT));
            }
        }
        // If T is an std::vector<block>
        else if constexpr (is_vector_block<T>::value) {
            uint32_t count = static_cast<uint32_t>(data.size());
            file.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));
            if (count > 0) {
                file.write(reinterpret_cast<const char *>(data.data()), count * sizeof(block));
            }
        }
        // If T is an std::array<arithmetic, N>
        else if constexpr (is_array_arith<T>::value) {
            constexpr size_t N = std::tuple_size<T>::value;
            using ElemT        = typename T::value_type;
            uint32_t count     = static_cast<uint32_t>(N);
            file.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));
            file.write(reinterpret_cast<const char *>(data.data()), N * sizeof(ElemT));
        }
        // If T is an std::array<block, N>
        else if constexpr (is_array_block<T>::value) {
            constexpr size_t N     = std::tuple_size<T>::value;
            uint32_t         count = static_cast<uint32_t>(N);
            file.write(reinterpret_cast<const char *>(&count), sizeof(uint32_t));
            file.write(reinterpret_cast<const char *>(data.data()),
                       N * sizeof(block));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported data type for binary writing.");
        }
        // ofstream is closed automatically when it goes out of scope
    }

    /**
     * @brief Reads data from a file in binary mode.
     * @tparam T Enabled only if is_binary_writable<T>::value == true
     * @param file_path The file name (without extension).
     * @param data The variable to store the read data.
     */
    template <typename T>
    void ReadBinary(const std::string &file_path, T &data) {
        // Compose full path with binary extension
        std::string full_path = AddExtension(file_path);

        // Open in binary mode
        std::ios_base::openmode mode = std::ios::in | std::ios::binary;
        std::ifstream           file(full_path, mode);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for binary reading: " + full_path);
        }

        // Enable exceptions on read failures
        file.exceptions(std::ios::badbit | std::ios::failbit);

        // Read the number of elements (as uint32_t for portability)
        uint32_t count = 0;
        file.read(reinterpret_cast<char *>(&count), sizeof(uint32_t));

        // If T is a single arithmetic value
        if constexpr (std::is_arithmetic_v<T>) {
            if (count != 1) {
                throw std::runtime_error("Unexpected count for single value: " + full_path);
            }
            file.read(reinterpret_cast<char *>(&data), sizeof(T));
        }
        // If T is a single block
        else if constexpr (std::is_same_v<T, block>) {
            if (count != 1) {
                throw std::runtime_error("Unexpected count for single block: " + full_path);
            }
            file.read(reinterpret_cast<char *>(&data), sizeof(block));
        }
        // If T is a std::string
        else if constexpr (std::is_same_v<T, std::string>) {
            data.resize(count);
            if (count > 0) {
                file.read(reinterpret_cast<char *>(data.data()), count);
            }
        }
        // If T is a std::vector<arithmetic>
        else if constexpr (is_vector_arith<T>::value) {
            using ElemT = typename T::value_type;
            data.resize(static_cast<size_t>(count));
            if (count > 0) {
                file.read(reinterpret_cast<char *>(data.data()), count * sizeof(ElemT));
            }
        }
        // If T is a std::vector<block>
        else if constexpr (is_vector_block<T>::value) {
            data.resize(static_cast<size_t>(count));
            if (count > 0) {
                file.read(reinterpret_cast<char *>(data.data()), count * sizeof(block));
            }
        }
        // If T is an std::array<arithmetic, N>
        else if constexpr (is_array_arith<T>::value) {
            constexpr size_t N = std::tuple_size<T>::value;
            if (count != N) {
                throw std::runtime_error("Size mismatch for std::array: " + full_path);
            }
            file.read(reinterpret_cast<char *>(data.data()), N * sizeof(typename T::value_type));
        }
        // If T is an std::array<block, N>
        else if constexpr (is_array_block<T>::value) {
            constexpr size_t N = std::tuple_size<T>::value;
            if (count != N) {
                throw std::runtime_error("Size mismatch for std::array<block>: " + full_path);
            }
            file.read(reinterpret_cast<char *>(data.data()), N * sizeof(block));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported data type for binary reading.");
        }
        // ofstream is closed automatically when it goes out of scope
    }

    /**
     * @brief Writes std::vector<std::string> data to a file.
     * @param file_path The file name (without extension).
     * @param data The data to be written.
     * @param append If true, appends data to the file. Otherwise, overwrites it. Defaults to false.
     */
    void WriteTextToFile(const std::string &file_path, const std::vector<std::string> &data, bool append = false) {
        // Compose full path with text extension
        std::string full_path = AddExtension(file_path);

        // Open the file in text mode
        std::ofstream           file;
        std::ios_base::openmode mode = std::ios::out;
        if (append) {
            mode |= std::ios::app;
        }
        file.open(full_path, mode);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + full_path);
        }

        // First line: number of elements
        file << data.size() << "\n";
        // Write each string on a new line
        for (const auto &line : data) {
            file << line << "\n";
        }
        // ofstream is closed automatically when it goes out of scope
    }

    /**
     * @brief Clears the contents of a specified file.
     * @param file_path The file name (without extension).
     */
    void ClearFileContents(const std::string &file_path) {
        std::ofstream file(AddExtension(file_path), std::ios::trunc);    // Truncate mode clears the content
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for clearing: " + AddExtension(file_path));
        }
        // ofstream is closed automatically when it goes out of scope
    }

private:
    const std::string ext_; /**< Default file extension. */

    std::string AddExtension(const std::string &file_path) const {
        return file_path + ext_;
    }
};

}    // namespace fsswm

#endif    // UTILS_FILE_IO_H_
