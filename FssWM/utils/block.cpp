#include "block.h"

#include <iomanip>

#include "rng.h"

namespace fsswm {

// Function to convert an emp::block to a string in the specified format
std::string ToString(const block &blk, FormatType format) {
    // Retrieve block data as two 64-bit values
    uint64_t *data = (uint64_t *)&blk;
    uint64_t  high = data[1];    // Higher 64 bits
    uint64_t  low  = data[0];    // Lower 64 bits

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

        case FormatType::kDec:    // Decimal (64-bit groups)
            oss << std::dec << high << " " << low;
            break;

        default:
            throw std::invalid_argument("Unsupported format type");
    }

    return oss.str();
}

bool Equal(const block &lhs, const block &rhs) {
    __m128i cmp = _mm_xor_si128(lhs, rhs);
    return _mm_testz_si128(cmp, cmp);
}

block SetRandomBlock() {
    return _mm_set_epi64x(SecureRng::Rand64(), SecureRng::Rand64());
}

}    // namespace fsswm
