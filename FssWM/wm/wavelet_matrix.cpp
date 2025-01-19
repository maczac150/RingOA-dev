#include "wavelet_matrix.h"

namespace fsswm {
namespace wm {

WaveletMatrix::WaveletMatrix(const std::vector<uint32_t> &data) {
    length_ = data.size();
    bitvecs_.resize(BITS);
    zero_counts_.resize(BITS);

    // We'll build from the highest bit to the lowest bit.
    // But in this sample, for convenience, we do loop from BITS -1 down to 0.
    std::vector<uint32_t> cur = data;      // current data
    std::vector<uint32_t> nxt(length_);    // temp for rearranging

    // level=BITS-1 -> most significant bit, level=0 -> least significant bit
    // However, we store them in bitvecs_[BITS -1 -level] in this code
    // so that bitvecs_[0] stores the most significant bit.
    for (int level = BITS - 1; level >= 0; --level) {
        BitVector bv(length_);
        size_t    zero_count = 0;

        // 1) Determine which elements have bit=1 at 'level'
        for (size_t i = 0; i < length_; ++i) {
            // mask for the current bit
            uint32_t mask = 1U << level;
            bool     bit  = (cur[i] & mask) != 0U;
            // if (cur[i]'s bit is 1 at 'level') => set bit in bv
            if (bit) {
                bv.set(i);
            } else {
                zero_count++;
            }
        }

        bv.build();
        // Store the bit vector and zero count at the index (BITS - 1 - level)
        bitvecs_[BITS - 1 - level]     = bv;
        zero_counts_[BITS - 1 - level] = zero_count;

        // 2) Reorder elements in 'cur' into 'nxt': first zeros, then ones
        size_t zero_pos = 0;
        size_t one_pos  = zero_count;
        for (size_t i = 0; i < length_; ++i) {
            uint32_t mask = 1U << level;
            bool     bit  = (cur[i] & mask) != 0U;
            if (bit) {
                nxt[one_pos++] = cur[i];
            } else {
                nxt[zero_pos++] = cur[i];
            }
        }
        // swap cur and nxt
        std::swap(cur, nxt);
    }
}

size_t WaveletMatrix::size() const {
    return length_;
}

uint32_t WaveletMatrix::access(size_t pos) const {
    uint32_t value = 0;
    // 'idx' tracks the index within the subarray of each level
    size_t idx = pos;

    for (int level = 0; level < BITS; ++level) {
        const BitVector &bv  = bitvecs_[level];
        size_t           zc  = zero_counts_[level];
        bool             bit = bv[idx];

        // If the bit at idx is 1, then the original value has a 1 at this bit level
        // (noting that level=0 is top bit in bitvecs_; we need to invert when we set the actual bit).
        if (bit) {
            // The real bit position is (BITS - 1 - level)
            value |= (1U << (BITS - 1 - level));
            // Now we move 'idx' into the 1-group region
            // The number of ones up to idx is bv.rank1(idx)
            // The offset int he 1-group is that rank1 value
            // plus the start offset which is the zero group size (zc).
            idx = zc + bv.rank1(idx);
        } else {
            // If bit=0, the original value has 0 at this bit level
            // Move idx into the 0-group region:
            // the offset is simply how many 0 bits have occurred, i.e. bv.rank0(idx)
            idx = bv.rank0(idx);
        }
    }
    return value;
}

uint32_t WaveletMatrix::rank(uint32_t c, size_t pos) const {
    if (pos > length_)
        pos = length_;
    if (pos == 0)
        return 0;

    // We'll track a range [left, right) that initially is [0, pos)
    size_t left = 0, right = pos;

    for (int level = 0; level < BITS; ++level) {
        if (left == right)
            return 0;    // no elements in range

        // We look at bit (BITS - 1 - level) in c
        // because level=0 corresponds to the top bit vector in this code.
        int b = (c >> (BITS - 1 - level)) & 1;

        const BitVector &bv = bitvecs_[level];
        size_t           zc = zero_counts_[level];

        if (b == 0) {
            // Restrict range to the 0-group
            // rank0 for left an  right, difference = how many 0s in [left, right)
            size_t left0  = bv.rank0(left);
            size_t right0 = bv.rank0(right);
            left          = left0;
            right         = right0;
        } else {
            // Restrict range to the 1-group
            // rank1 for left and right, difference = how many 1s in [left, right)
            // plus offset zc to map it to the 1-group index space
            size_t left1  = bv.rank1(left);
            size_t right1 = bv.rank1(right);
            left          = zc + left1;
            right         = zc + right1;
        }
    }
    // After going through all bits, [left, right) are exactly the positions of c
    // so the count is right - left
    return right - left;
}

}    // namespace wm
}    // namespace fsswm
