#ifndef UTILS_FORMAT_TYPES_H_
#define UTILS_FORMAT_TYPES_H_

#include <cstdint>    // std::size_t

namespace fsswm {

// Define format types
enum class FormatType
{
    kBin,    // Binary
    kHex,    // Hexadecimal
    kDec,    // Decimal
};

// Default max size for string conversions
constexpr std::size_t kSizeMax = 30;

}    // namespace fsswm

#endif    // UTILS_FORMAT_TYPES_H_
