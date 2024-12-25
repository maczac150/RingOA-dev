#include "fss.hpp"

#include "utils/rng.hpp"

namespace fsswm {
namespace fss {

// Function to convert an emp::block to a string in the specified format
std::string ToString(const emp::block &blk, FormatType format) {
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
    return _mm_set_epi64x(utils::SecureRng::Rand64(), utils::SecureRng::Rand64());
}

uint32_t Convert(const block &b, const uint32_t bitsize) {
    return _mm_cvtsi128_si32(b) & ((1U << bitsize) - 1U);
}

void CovertVector(const block &b, const uint32_t split_bit, const uint32_t bitsize, std::vector<uint32_t> &output) {
    uint32_t mask = (1U << bitsize) - 1U;
    if (split_bit == 2) {
        alignas(16) uint32_t data32[4];
        _mm_store_si128(reinterpret_cast<__m128i *>(data32), b);
        for (uint32_t i = 0; i < 4; i++) {
            output[i] = data32[i] & mask;
        }
    } else if (split_bit == 3) {
        alignas(16) uint16_t data16[8];
        _mm_store_si128(reinterpret_cast<__m128i *>(data16), b);
        for (uint32_t i = 0; i < 8; i++) {
            output[i] = data16[i] & mask;
        }
    } else if (split_bit == 7) {
        uint8_t bytes[16];
        _mm_store_si128((__m128i *)bytes, b);
        for (int i = 0; i < 128; ++i) {
            uint8_t byte = bytes[i / 8];
            output[i]    = (byte >> (i % 8)) & 0x01 & mask;
        }
    } else {
        utils::Logger::FatalLog(LOC, "Unsupported spilt bit: " + std::to_string(split_bit));
        exit(EXIT_FAILURE);
    }
}

uint32_t GetValueFromSplitBlock(block &b, const uint32_t split_bit, const uint32_t idx) {
    if (split_bit == 2) {
        alignas(16) uint32_t data32[4];
        _mm_store_si128(reinterpret_cast<__m128i *>(data32), b);
        return data32[idx];
    } else if (split_bit == 3) {
        alignas(16) uint16_t data16[8];
        _mm_store_si128(reinterpret_cast<__m128i *>(data16), b);
        return data16[idx];
    } else if (split_bit == 7) {
        uint64_t low  = _mm_extract_epi64(b, 0);
        uint64_t high = _mm_extract_epi64(b, 1);
        if (idx < 64) {
            return (low >> idx) & 1;
        } else {
            return (high >> (idx - 64)) & 1;
        }
    } else {
        utils::Logger::FatalLog(LOC, "Unsupported spilt bit: " + std::to_string(split_bit));
        exit(EXIT_FAILURE);
    }
}

}    // namespace fss
}    // namespace fsswm
