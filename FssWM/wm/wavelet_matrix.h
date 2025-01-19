#ifndef WAVELET_MATRIX_H_
#define WAVELET_MATRIX_H_

#include <cstdint>
#include <vector>

#include "bit_vector.h"

namespace fsswm {
namespace wm {

class WaveletMatrix {
public:
    // Constructor: build wavelet matrix from data
    WaveletMatrix() = default;
    WaveletMatrix(const std::vector<uint32_t> &data);

    // size of the original array
    size_t size() const;

    // access(i): get the original value at index i (0-indexed)
    uint32_t access(size_t pos) const;

    // rank(c, pos): count how many times c appears in the range [0, pos)
    uint32_t rank(uint32_t c, size_t pos) const;

private:
    static const int BITS = 32; /**< for up to 32-bit integers */

    std::vector<BitVector> bitvecs_;     /**< Bit vectors for each bit level */
    std::vector<size_t>    zero_counts_; /**< Number of zeros at each bit level */
    size_t                 length_;      /**< Number of elements in the original array */
};

}    // namespace wm
}    // namespace fsswm

#endif    // WAVELET_MATRIX_H_
