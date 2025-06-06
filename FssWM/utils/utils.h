#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <chrono>        // std::chrono::system_clock
#include <cmath>         // std::pow, std::log2
#include <filesystem>    // std::filesystem::current_path

namespace fsswm {

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
