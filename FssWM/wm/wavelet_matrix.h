#ifndef WM_WAVELET_MATRIX_H_
#define WM_WAVELET_MATRIX_H_

#include <array>
#include <cstdint>
#include <vector>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace wm {

class WaveletMatrix {
public:
    WaveletMatrix() = default;
    explicit WaveletMatrix(const std::vector<uint32_t> &data);

    size_t size() const;

    uint32_t RankCF(uint32_t c, size_t position) const;

private:
    static constexpr size_t                 BITS = 3;
    std::array<std::vector<uint32_t>, BITS> rank0_tables_;
    size_t                                  length_ = 0;

    void Build(const std::vector<uint32_t> &data);
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_WAVELET_MATRIX_H_
