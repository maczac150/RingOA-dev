#include "fss.h"

#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"

namespace fsswm {
namespace fss {

uint32_t Convert(const block &b, const uint32_t bitsize) {
    return _mm_cvtsi128_si32(b) & ((1U << bitsize) - 1U);
}

void ConvertVector(const block &b, const uint32_t split_bit, const uint32_t bitsize, std::vector<uint32_t> &output) {
    uint32_t mask = (1U << bitsize) - 1U;
    if (split_bit == 2) {
        alignas(16) uint32_t data32[4];
        _mm_store_si128(reinterpret_cast<__m128i *>(data32), b);
        for (uint32_t i = 0; i < 4; ++i) {
            output[i] = data32[i] & mask;
        }
    } else if (split_bit == 3) {
        alignas(16) uint16_t data16[8];
        _mm_store_si128(reinterpret_cast<__m128i *>(data16), b);
        for (uint32_t i = 0; i < 8; ++i) {
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
        Logger::FatalLog(LOC, "Unsupported spilt bit: " + std::to_string(split_bit));
        std::exit(EXIT_FAILURE);
    }
}

void ConvertVector(const std::vector<block> &b, const uint32_t split_bit, const uint32_t bitsize, std::vector<uint32_t> &output) {
    uint32_t mask            = (1U << bitsize) - 1U;
    uint32_t num_nodes       = b.size();
    uint32_t remaining_nodes = 1U << split_bit;

    if (split_bit == 2) {
        alignas(16) uint32_t data32[4];
        for (size_t i = 0; i < num_nodes; ++i) {
            _mm_store_si128(reinterpret_cast<__m128i *>(data32), b[i]);
            output[i * remaining_nodes + 0] = data32[0] & mask;
            output[i * remaining_nodes + 1] = data32[1] & mask;
            output[i * remaining_nodes + 2] = data32[2] & mask;
            output[i * remaining_nodes + 3] = data32[3] & mask;
        }
    } else if (split_bit == 3) {
        alignas(16) uint16_t data16[8];
        for (size_t i = 0; i < num_nodes; ++i) {
            _mm_store_si128(reinterpret_cast<__m128i *>(data16), b[i]);
            output[i * remaining_nodes + 0] = data16[0] & mask;
            output[i * remaining_nodes + 1] = data16[1] & mask;
            output[i * remaining_nodes + 2] = data16[2] & mask;
            output[i * remaining_nodes + 3] = data16[3] & mask;
            output[i * remaining_nodes + 4] = data16[4] & mask;
            output[i * remaining_nodes + 5] = data16[5] & mask;
            output[i * remaining_nodes + 6] = data16[6] & mask;
            output[i * remaining_nodes + 7] = data16[7] & mask;
        }
    } else {
        Logger::FatalLog(LOC, "Unsupported spilt bit: " + std::to_string(split_bit));
        std::exit(EXIT_FAILURE);
    }
}

void ConvertVector(const std::vector<block> &b1,
                   const std::vector<block> &b2,
                   const uint32_t            split_bit,
                   const uint32_t            bitsize,
                   std::vector<uint32_t>    &out1,
                   std::vector<uint32_t>    &out2) {
    uint32_t mask            = (1U << bitsize) - 1U;
    uint32_t num_nodes       = b1.size();
    uint32_t remaining_nodes = 1U << split_bit;

    if (split_bit == 2) {
        alignas(16) uint32_t data32[4];
        for (size_t i = 0; i < num_nodes; ++i) {
            _mm_store_si128(reinterpret_cast<__m128i *>(data32), b1[i]);
            out1[i * remaining_nodes + 0] = data32[0] & mask;
            out1[i * remaining_nodes + 1] = data32[1] & mask;
            out1[i * remaining_nodes + 2] = data32[2] & mask;
            out1[i * remaining_nodes + 3] = data32[3] & mask;
        }
        for (size_t i = 0; i < num_nodes; ++i) {
            _mm_store_si128(reinterpret_cast<__m128i *>(data32), b2[i]);
            out2[i * remaining_nodes + 0] = data32[0] & mask;
            out2[i * remaining_nodes + 1] = data32[1] & mask;
            out2[i * remaining_nodes + 2] = data32[2] & mask;
            out2[i * remaining_nodes + 3] = data32[3] & mask;
        }
    } else if (split_bit == 3) {
        alignas(16) uint16_t data16[8];
        for (size_t i = 0; i < num_nodes; ++i) {
            _mm_store_si128(reinterpret_cast<__m128i *>(data16), b1[i]);
            out1[i * remaining_nodes + 0] = data16[0] & mask;
            out1[i * remaining_nodes + 1] = data16[1] & mask;
            out1[i * remaining_nodes + 2] = data16[2] & mask;
            out1[i * remaining_nodes + 3] = data16[3] & mask;
            out1[i * remaining_nodes + 4] = data16[4] & mask;
            out1[i * remaining_nodes + 5] = data16[5] & mask;
            out1[i * remaining_nodes + 6] = data16[6] & mask;
            out1[i * remaining_nodes + 7] = data16[7] & mask;
        }
        for (size_t i = 0; i < num_nodes; ++i) {
            _mm_store_si128(reinterpret_cast<__m128i *>(data16), b2[i]);
            out2[i * remaining_nodes + 0] = data16[0] & mask;
            out2[i * remaining_nodes + 1] = data16[1] & mask;
            out2[i * remaining_nodes + 2] = data16[2] & mask;
            out2[i * remaining_nodes + 3] = data16[3] & mask;
            out2[i * remaining_nodes + 4] = data16[4] & mask;
            out2[i * remaining_nodes + 5] = data16[5] & mask;
            out2[i * remaining_nodes + 6] = data16[6] & mask;
            out2[i * remaining_nodes + 7] = data16[7] & mask;
        }
    } else {
        Logger::FatalLog(LOC, "Unsupported spilt bit: " + std::to_string(split_bit));
        std::exit(EXIT_FAILURE);
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
        Logger::FatalLog(LOC, "Unsupported spilt bit: " + std::to_string(split_bit));
        std::exit(EXIT_FAILURE);
    }
}

}    // namespace fss
}    // namespace fsswm
