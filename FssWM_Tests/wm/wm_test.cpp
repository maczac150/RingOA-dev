#include "wm_test.h"

#include <sdsl/rank_support.hpp>
#include <sdsl/wavelet_trees.hpp>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/bit_vector.h"
#include "FssWM/wm/fmindex.h"
#include "FssWM/wm/wavelet_matrix.h"

namespace {

// This function counts how many times 'c' appears in [0, pos).
static uint32_t naiveRank(const std::vector<uint32_t> &data, uint32_t c, size_t pos) {
    if (pos > data.size())
        pos = data.size();
    uint32_t count = 0;
    for (size_t i = 0; i < pos; i++) {
        if (data[i] == c)
            count++;
    }
    return count;
}

}    // namespace

namespace test_fsswm {

using fsswm::Logger;
using fsswm::ToString;
using fsswm::wm::BitVector;
using fsswm::wm::FMIndex;
using fsswm::wm::WaveletMatrix;

void BitVector_Test() {
    Logger::DebugLog(LOC, "BitVector_Test...");

    size_t    n = 20;
    BitVector bv(n);

    std::vector<size_t> set_positions = {2, 5, 7, 10, 15};
    for (auto pos : set_positions) {
        bv.set(pos);
    }

    bv.build();

    std::vector<bool> values;
    for (size_t i = 0; i < n; i++) {
        values.push_back(bv[i]);
    }

    Logger::DebugLog(LOC, "values: " + ToString(values));

    std::vector<size_t> test_positions = {0, 5, 10, 20};
    for (auto pos : test_positions) {
        uint32_t r1 = bv.rank1(pos);
        uint32_t r0 = bv.rank0(pos);
        Logger::DebugLog(LOC, "rank1(" + std::to_string(pos) + "): " + std::to_string(r1));
        Logger::DebugLog(LOC, "rank0(" + std::to_string(pos) + "): " + std::to_string(r0));
    }

    Logger::DebugLog(LOC, "BitVector_Test - Passed");
}

void WaveletMatrix_Test() {
    Logger::DebugLog(LOC, "WaveletMatrix_Test...");

    std::vector<uint32_t> data = {5, 2, 2, 7, 5, 9};

    // Build wavelet matrix
    WaveletMatrix wm(data);

    // Check access
    for (size_t i = 0; i < data.size(); i++) {
        uint32_t val_wm    = wm.access(i);
        uint32_t val_naive = data[i];
        Logger::DebugLog(LOC, "val_wm: " + std::to_string(val_wm) + ", val_naive: " + std::to_string(val_naive));
        if (val_wm != val_naive) {
            Logger::ErrorLog(LOC, "val_wm != val_naive");
        }
    }

    // Check rank
    uint32_t c = 5;
    for (size_t i = 0; i <= data.size(); i++) {
        uint32_t rank_wm    = wm.rank(c, i);
        uint32_t rank_naive = naiveRank(data, c, i);
        Logger::DebugLog(LOC, "rank_wm: " + std::to_string(rank_wm) + ", rank_naive: " + std::to_string(rank_naive));
        if (rank_wm != rank_naive) {
            Logger::ErrorLog(LOC, "rank_wm != rank_naive");
        }
    }

    Logger::DebugLog(LOC, "data: " + ToString(data));

    Logger::DebugLog(LOC, "WaveletMatrix_Test - Passed");
}

void FMIndex_Test() {
    Logger::DebugLog(LOC, "FMIndex_Test...");

    std::string dna = "GATTACA";
    // Build FM-index
    FMIndex fm(dna);

    std::string query = "TA";
    size_t      c     = fm.count(query);
    Logger::DebugLog(LOC, "count(" + query + "): " + std::to_string(c));

    auto locs = fm.locate(query);
    Logger::DebugLog(LOC, "locate(" + query + "): " + ToString(locs));

    Logger::DebugLog(LOC, "FMIndex_Test - Passed");
}

}    // namespace test_fsswm
