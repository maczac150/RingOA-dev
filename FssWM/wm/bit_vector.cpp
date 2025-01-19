#include "bit_vector.h"

#include "FssWM/utils/utils.h"
#include "FssWM/utils/logger.h"

namespace fsswm {
namespace wm {

BitVector::BitVector()
    : n_(0) {
}

BitVector::BitVector(size_t size)
    : n_(size) {
    size_t block_count = (size + 63) >> 6;
    bits_.assign(block_count, 0ULL);
    popcount_.assign(block_count + 1, 0U);
    Logger::DebugLog(LOC, "BitVector: " + ToString(bits_));
}

void BitVector::set(size_t i) {
    bits_[i >> 6] |= (1ULL << (i & 63));
}

void BitVector::build() {
    for (size_t i = 0; i < bits_.size(); ++i) {
        popcount_[i + 1] = popcount_[i] + __builtin_popcountll(bits_[i]);
    }
}

bool BitVector::operator[](size_t i) const {
    return (bits_[i >> 6] >> (i & 63)) & 1ULL;
}

uint32_t BitVector::rank1(size_t pos) const {
    if (pos > n_)
        pos = n_;
    size_t   block_idx = pos >> 6;
    uint64_t mask      = (1ULL << (pos & 63)) - 1;
    uint64_t block     = bits_[block_idx] & mask;
    return popcount_[block_idx] + __builtin_popcountll(block);
}

uint32_t BitVector::rank0(size_t pos) const {
    return pos - rank1(pos);
}

size_t BitVector::size() const {
    return n_;
}

}    // namespace wm
}    // namespace fsswm
