#ifndef WM_BIT_VECTOR_H_
#define WM_BIT_VECTOR_H_

#include <cstdint>
#include <vector>

namespace fsswm {
namespace wm {

class BitVector {
public:
    BitVector();
    BitVector(size_t size);
    void     set(size_t i);
    void     build();
    bool     operator[](size_t i) const;
    uint32_t rank1(size_t pos) const;
    uint32_t rank0(size_t pos) const;
    size_t   size() const;

private:
    std::vector<uint64_t> bits_;      /**< 64-bit blocks */
    std::vector<uint32_t> popcount_; /**< Cumulative sum of popcounts */
    size_t                n_;         /**< Total number of bits */
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_BIT_VECTOR_H_
