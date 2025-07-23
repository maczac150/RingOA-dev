#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <chrono>        // std::chrono::system_clock
#include <cmath>         // std::pow, std::log2
#include <filesystem>    // std::filesystem::current_path

namespace ringoa {

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
    return value & ((1UL << bitsize) - 1UL);
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
 * @brief Gets the least significant bit (LSB) of a given value.
 * @param value The input value.
 * @return The LSB (0 or 1).
 */
inline uint64_t GetLSB(const uint64_t value) noexcept {
    return value & 1ULL;
}

/**
 * @brief Gets the most significant bit (MSB) of a given n-bit value.
 * @param value The input value.
 * @param n The bit width (MSB is at position n - 1).
 * @return The MSB (0 or 1).
 */
inline uint64_t GetMSB(const uint64_t value, const uint64_t n) noexcept {
    return (value >> (n - 1)) & 1ULL;
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
 * @brief Converts an unsigned n-bit value to its signed equivalent using sign extension.
 * @param value The unsigned input value (lower n bits are significant).
 * @param n The bit width of the value (1 <= n <= 64).
 * @return The signed interpretation of the n-bit value.
 */
inline int64_t UnsignedToSignedNBits(uint64_t value, uint64_t n) noexcept {
    // Extract only the lower n bits
    value &= (1ULL << n) - 1;

    // If MSB is 1, do sign extension
    if (value >> (n - 1)) {
        // Negative value: extend the sign bit
        return static_cast<int64_t>(value | (~0ULL << n));
    } else {
        // Positive value: no change needed
        return static_cast<int64_t>(value);
    }
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

}    // namespace ringoa

#endif    // UTILS_UTILS_H_
