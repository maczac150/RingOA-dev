#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace fsswm {
namespace utils {

/**
 * @brief Get the Current Date Time As String object
 * @return std::string The current date time as string
 */
std::string GetCurrentDateTimeAsString();

/**
 * @brief Gets the current working directory.
 * @return A string containing the current directory path.
 */
std::string GetCurrentDirectory();

/**
 * @brief Creates a sequence of numbers within a given range.
 * @param start The starting value of the sequence (inclusive).
 * @param end The ending value of the sequence (exclusive).
 * @return A vector containing the sequence of numbers.
 */
std::vector<uint32_t> CreateSequence(const uint32_t start, const uint32_t end);

/**
 * @brief Converts a container (std::array or std::vector) to a string with specified delimiter.
 * @tparam Container The type of the container (e.g., std::array, std::vector).
 * @param container The container to be converted to a string.
 * @param del The delimiter string used to separate elements in the output string.
 * @return A string representation of the container elements separated by 'del'.
 */
template <typename Container>
std::string ToString(const Container &container, const std::string &del = " ") {
    std::stringstream ss;
    for (std::size_t i = 0; i < container.size(); ++i) {
        ss << container[i];
        if (i != container.size() - 1) {
            ss << del;
        }
    }
    return ss.str();
}

/**
 * @brief Converts a boolean vector to a string representation.
 * @param bool_vector The input boolean vector to be converted.
 * @return A string representation of the boolean vector.
 */
std::string ToString(const std::vector<bool> &bool_vector);

/**
 * @brief Converts a double value to a string representation with specified precision.
 * @param val The double value to be converted to a string.
 * @param digits The number of digits after the decimal point in the resulting string.
 * @return A string representation of the double value with the specified precision.
 */
std::string ToString(const double val, const size_t digits = 0);

/**
 * @brief Converts an array of bytes to a hexadecimal string representation.
 * @param data Pointer to the array of bytes.
 * @param length The length of the array 'data'.
 * @return A string representing the hexadecimal conversion of the byte array.
 */
std::string ConvertToHex(unsigned char *data, size_t length);

// #############################
// ######## Calculation ########
// #############################

/**
 * @brief Calculates the power of an integer to another integer exponent.
 * @param base The base value.
 * @param exponent The exponent value.
 * @return The result of 'base' raised to the power of 'exponent'.
 */
inline uint32_t Pow(const uint32_t base, const uint32_t exponent) {
    return static_cast<uint32_t>(std::pow(base, exponent));
}

/**
 * @brief Performs modular arithmetic on a value using a specified bit size.
 * @param value The value on which modulo operation is performed.
 * @param bitsize The bit size used for computing the modulo.
 * @return The result of 'value' modulo (2^bitsize).
 */
inline uint32_t Mod(const uint32_t value, const uint32_t bitsize) {
    return value & ((1U << bitsize) - 1U);
}

/**
 * @brief Gets the lower 'n' bits of a given value.
 * @param value The value from which the lower bits are obtained.
 * @param n The number of lower bits to be retrieved.
 * @return The lower 'n' bits of the input value.
 */
uint32_t GetLowerNBits(const uint32_t value, const uint32_t n);

}    // namespace utils
}    // namespace fsswm

#endif    // UTILS_UTILS_H_
