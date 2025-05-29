#include "fss.h"

#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"

namespace fsswm {
namespace fss {

uint64_t Convert(const block &b, const uint64_t bitsize) {
    return b.get<uint64_t>()[0] & ((1U << bitsize) - 1U);
}

void SplitBlockToFieldVector(
    const block           &blk,
    uint64_t               chunk_exp,
    uint64_t               field_bits,
    std::vector<uint64_t> &out) {

    const size_t chunk_count = size_t{1} << chunk_exp;

    if (chunk_exp != 2 && chunk_exp != 3 && chunk_exp != 7) {
        throw std::invalid_argument("Unsupported chunk_exp: " + ToString(chunk_exp));
    }

    out.assign(chunk_count, 0);
    const uint64_t mask = (1UL << field_bits) - 1;

    alignas(16) uint8_t raw_bytes[16];
    _mm_store_si128(reinterpret_cast<__m128i *>(raw_bytes), blk);

    switch (chunk_exp) {
        case 2: {
            // 32bit element ×4
            auto data32 = reinterpret_cast<const uint32_t *>(raw_bytes);
            for (size_t i = 0; i < 4; ++i) {
                out[i] = uint64_t(data32[i]) & mask;
            }
        } break;
        case 3: {
            // 16bit element ×8
            auto data16 = reinterpret_cast<const uint16_t *>(raw_bytes);
            for (size_t i = 0; i < 8; ++i) {
                out[i] = uint64_t(data16[i]) & mask;
            }
        } break;
        case 7: {
            // 1bit element ×128
            for (size_t bit = 0; bit < 128; ++bit) {
                uint8_t  byte   = raw_bytes[bit / 8];
                uint64_t bitval = uint64_t((byte >> (bit & 7)) & 1);
                out[bit]        = bitval & mask;
            }
        } break;
        default:
            throw std::invalid_argument("Unsupported chunk_exp: " + ToString(chunk_exp));
    }
}

void SplitBlockToFieldVector(
    const std::vector<block> &blks,
    uint64_t                  chunk_exp,
    uint64_t                  field_bits,
    std::vector<uint64_t>    &out) {
    const size_t   num_blocks  = blks.size();
    const size_t   chunk_count = size_t{1} << chunk_exp;
    const uint64_t mask        = (field_bits >= 64)
                                     ? ~0ULL
                                     : ((1ULL << field_bits) - 1ULL);

    out.resize(num_blocks * chunk_count);

    alignas(16) uint8_t raw_bytes[16];

    switch (chunk_exp) {
        case 2: {    // 32bit ×4
            for (size_t i = 0; i < num_blocks; ++i) {
                _mm_store_si128(reinterpret_cast<__m128i *>(raw_bytes), blks[i]);
                auto   data32 = reinterpret_cast<const uint32_t *>(raw_bytes);
                size_t base   = i * chunk_count;
                out[base + 0] = data32[0] & mask;
                out[base + 1] = data32[1] & mask;
                out[base + 2] = data32[2] & mask;
                out[base + 3] = data32[3] & mask;
            }
            break;
        }
        case 3: {    // 16bit ×8
            for (size_t i = 0; i < num_blocks; ++i) {
                _mm_store_si128(reinterpret_cast<__m128i *>(raw_bytes), blks[i]);
                auto   data16 = reinterpret_cast<const uint16_t *>(raw_bytes);
                size_t base   = i * chunk_count;
                out[base + 0] = data16[0] & mask;
                out[base + 1] = data16[1] & mask;
                out[base + 2] = data16[2] & mask;
                out[base + 3] = data16[3] & mask;
                out[base + 4] = data16[4] & mask;
                out[base + 5] = data16[5] & mask;
                out[base + 6] = data16[6] & mask;
                out[base + 7] = data16[7] & mask;
            }
            break;
        }
        default:
            throw std::invalid_argument("Unsupported chunk_exp: " + ToString(chunk_exp));
    }
}

uint64_t GetSplitBlockValue(
    const block &blk,
    uint64_t     chunk_exp,
    uint64_t     element_idx) {

    const size_t count = size_t{1} << chunk_exp;
    if (element_idx >= count)
        throw std::out_of_range("element_idx out of range");

    alignas(16) uint8_t bytes[16];
    _mm_store_si128(reinterpret_cast<__m128i *>(bytes), blk);

    switch (chunk_exp) {
        case 2: {    // 4 element × 32bit
            auto data32 = reinterpret_cast<const uint32_t *>(bytes);
            return uint64_t(data32[element_idx]);
        }
        case 3: {    // 8 element × 16bit
            auto data16 = reinterpret_cast<const uint16_t *>(bytes);
            return uint64_t(data16[element_idx]);
        }
        case 7: {    // 128 element × 1bit
            uint64_t byte_idx   = element_idx % 16;
            uint64_t bit_idx    = element_idx / 16;
            auto     seed_bytes = reinterpret_cast<const uint8_t *>(&blk);
            return (seed_bytes[byte_idx] >> bit_idx) & 0x01;
        }
        default:
            throw std::invalid_argument("Unsupported chunk_exp: " + ToString(chunk_exp));
    }
}

}    // namespace fss
}    // namespace fsswm
