#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <bitset>        // std::bitset
#include <chrono>        // std::chrono::system_clock
#include <cmath>         // std::pow, std::log2 など
#include <cstdint>       // uint64_t, size_t
#include <filesystem>    // std::filesystem::current_path
#include <iomanip>       // std::setprecision, std::put_time
#include <ranges>        // std::ranges::*
#include <span>          // std::span
#include <sstream>       // std::ostringstream
#include <string>        // std::string, std::string_view
#include <vector>        // std::vector

#include "block.h"

namespace fsswm {

// #############################
// ####### String Utils ########
// #############################
constexpr size_t kSizeMax = 64;

// Define format types
enum class FormatType
{
    kBin,    // Binary
    kHex,    // Hexadecimal
};

// ――――――――――――――――――――――
//  scalar overloads
// ――――――――――――――――――――――
inline std::string ToString(std::string_view sv) {
    return std::string{sv};
}

template <std::integral I>
std::string ToString(I v) {
    return std::to_string(v);
}

template <std::floating_point F>
std::string ToString(F v) {
    return std::to_string(v);
}

inline std::string ToString(double v, size_t digits) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(digits) << v;
    std::string res = ss.str();
    auto        pos = res.find('.');
    if (pos == std::string::npos) {
        if (digits > 0) {
            res += '.';
            res += std::string(digits, '0');
        }
    } else {
        size_t cur = res.size() - pos - 1;
        if (cur < digits) {
            res += std::string(digits - cur, '0');
        }
    }
    return res;
}

inline std::string ToString(const std::vector<bool> &bv) {
    std::string s;
    s.reserve(bv.size());
    for (bool b : bv) {
        s.push_back(b ? '1' : '0');
    }
    return s;
}

inline std::string ToString(const block &blk, FormatType format = FormatType::kHex) {
    // Retrieve block data as two 64-bit values
    auto     arr  = blk.get<uint64_t>();
    uint64_t low  = arr[0];
    uint64_t high = arr[1];

    std::ostringstream oss;
    switch (format) {
        case FormatType::kBin:    // Binary (32-bit groups)
            oss << std::bitset<64>(high).to_string().substr(0, 32) << " "
                << std::bitset<64>(high).to_string().substr(32, 32) << " "
                << std::bitset<64>(low).to_string().substr(0, 32) << " "
                << std::bitset<64>(low).to_string().substr(32, 32);
            break;
        case FormatType::kHex:    // Hexadecimal (64-bit groups)
            oss << std::hex << std::setw(16) << std::setfill('0') << high << " "
                << std::setw(16) << std::setfill('0') << low;
            break;
        default:
            oss << std::dec << high << " " << low;
            break;
    }
    return oss.str();
}

// ――――――――――――――――――――――
//  span + FormatType (Bin/Hex only)
// ――――――――――――――――――――――
template <std::integral T>
std::string ToString(std::span<const T> data,
                     FormatType         fmt,
                     std::string_view   delim    = " ",
                     size_t             max_size = kSizeMax) {
    std::stringstream ss;
    size_t            n = std::min(data.size(), max_size);

    for (size_t i = 0; i < n; ++i) {
        // preserve bit‐pattern
        using U = std::make_unsigned_t<T>;
        U uval  = static_cast<U>(data[i]);

        switch (fmt) {
            case FormatType::kBin:
                ss << std::bitset<std::numeric_limits<T>::digits>(uval);
                break;
            case FormatType::kHex:
                ss << std::hex << std::uppercase << static_cast<unsigned>(uval) << std::dec;
                break;
        }
        if (i + 1 < n)
            ss << delim;
    }
    if (data.size() > max_size)
        ss << delim << "...";
    return ss.str();
}

// ――――――――――――――――――――――――
//  span without FormatType (decimal)
// ――――――――――――――――――――――――
template <typename T>
std::string ToString(std::span<const T> data,
                     std::string_view   delim    = " ",
                     size_t             max_size = kSizeMax) {
    size_t      n = std::min(data.size(), max_size);
    std::string out;
    out.reserve(n * 8);
    for (size_t i = 0; i < n; ++i) {
        out += ToString(data[i]);
        if (i + 1 < n)
            out += delim;
    }
    if (data.size() > max_size)
        out += delim, out += "...";
    return "[" + out + "]";
}

// ――――――――――――――――――――――
//  contiguous_range overloads
// ――――――――――――――――――――――
template <std::ranges::contiguous_range R>
    requires std::ranges::sized_range<R> && std::integral<std::ranges::range_value_t<R>>
std::string ToString(const R         &r,
                     FormatType       fmt,
                     std::string_view delim    = " ",
                     size_t           max_size = kSizeMax) {
    return ToString(std::span{std::ranges::data(r), std::ranges::size(r)},
                    fmt, delim, max_size);
}

template <std::ranges::contiguous_range R>
    requires std::ranges::sized_range<R>
std::string ToString(const R         &r,
                     std::string_view delim    = " ",
                     size_t           max_size = kSizeMax) {
    return ToString(std::span{std::ranges::data(r), std::ranges::size(r)},
                    delim, max_size);
}

// ――――――――――――――――――――――
//  Matrix: flat representation (Decimal only)
// ――――――――――――――――――――――
template <typename T>
std::string ToStringFlatMat(
    std::span<const T> flat,
    size_t             rows,
    size_t             cols,
    std::string_view   row_pref = "[",
    std::string_view   row_suff = "]",
    std::string_view   col_del  = " ",
    std::string_view   row_del  = ",") {
    if (flat.size() != rows * cols) {
        throw std::invalid_argument(
            "ToStringFlatMat(decimal): flat.size() != rows*cols");
    }
    std::ostringstream oss;
    for (size_t i = 0; i < rows; ++i) {
        oss << row_pref;
        for (size_t j = 0; j < cols; ++j) {
            oss << ToString(flat[i * cols + j]);    // scalar or container 用 ToString
            if (j + 1 < cols)
                oss << col_del;
        }
        oss << row_suff;
        if (i + 1 < rows)
            oss << row_del;
    }
    return oss.str();
}

// contiguous_range overload for flat matrix representation
template <std::ranges::contiguous_range R>
    requires std::ranges::sized_range<R>
std::string ToStringFlatMat(
    const R         &flat,
    size_t           rows,
    size_t           cols,
    std::string_view row_pref = "[",
    std::string_view row_suff = "]",
    std::string_view col_del  = " ",
    std::string_view row_del  = ",") {
    return ToStringFlatMat(
        std::span{std::ranges::data(flat), std::ranges::size(flat)},
        rows, cols,
        row_pref, row_suff, col_del, row_del);
}

// #############################
// ######## Calculation ########
// #############################

/**
 * @brief Calculates the power of an integer to another integer exponent.
 * @param base The base value.
 * @param exponent The exponent value.
 * @return The result of 'base' raised to the power of 'exponent'.
 */
inline uint64_t Pow(const uint64_t base, const uint64_t exponent) noexcept {
    return static_cast<uint64_t>(std::pow(base, exponent));
}

/**
 * @brief Performs modular arithmetic on a value using a specified bit size.
 * @param value The value on which modulo operation is performed.
 * @param bitsize The bit size used for computing the modulo.
 * @return The result of 'value' modulo (2^bitsize).
 */
inline uint64_t Mod(const uint64_t value, const uint64_t bitsize) noexcept {
    return value & ((1U << bitsize) - 1U);
}

/**
 * @brief Computes the sign of a boolean value.
 * @param b The boolean value.
 * @return -1 if 'b' is true, otherwise 1.
 */
inline int64_t Sign(bool b) noexcept {
    return b ? -1 : 1;
}

/**
 * @brief Compute floor(log2(x)).
 * @param x The input value.
 * @return floor(log2(x)) or -1 if x == 0.
 */
inline int log2floor(uint64_t x) noexcept {
    return x == 0 ? -1 : 31 - __builtin_clz(x);
}

/**
 * @brief Gets the lower 'n' bits of a given value.
 * @param value The value from which the lower bits are obtained.
 * @param n The number of lower bits to be retrieved.
 * @return The lower 'n' bits of the input value.
 */
inline uint64_t GetLowerNBits(const uint64_t value, const uint64_t n) noexcept {
    if (n >= 64) {
        return value;
    }
    return value & ((1ULL << n) - 1UL);
}

/**
 * @brief Get the Current Date Time As String object
 * @return std::string The current date time as string
 */
inline std::string GetCurrentDateTimeAsString() {
    auto              now         = std::chrono::system_clock::now();
    std::time_t       currentTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y%m%d_%H%M%S");
    return ss.str();
}

/**
 * @brief Gets the current working directory.
 * @return A string containing the current directory path.
 */
inline std::string GetCurrentDirectory() {
    return std::filesystem::current_path().string();
}

/**
 * @brief Creates a sequence of numbers within a given range.
 * @param start The starting value of the sequence (inclusive).
 * @param end The ending value of the sequence (exclusive).
 * @return A vector containing the sequence of numbers.
 */
inline std::vector<uint64_t> CreateSequence(const uint64_t start, const uint64_t end) {
    std::vector<uint64_t> sequence;
    sequence.reserve(end - start);
    for (uint64_t i = start; i < end; ++i) {
        sequence.push_back(i);
    }
    return sequence;
}

}    // namespace fsswm

#endif    // UTILS_UTILS_H_
