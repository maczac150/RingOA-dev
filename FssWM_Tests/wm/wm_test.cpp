#include "wm_test.h"

#include <sdsl/csa_wt.hpp>
#include <sdsl/suffix_arrays.hpp>

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/fmindex.h"
#include "FssWM/wm/wavelet_matrix.h"

namespace {

uint32_t ComputeLPMwithSDSL(const std::string &text, const std::string &query) {
    sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024> fm_sdsl;
    sdsl::construct_im(fm_sdsl, text, 1);

    typename sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024>::size_type l = 0, r = fm_sdsl.size() - 1;

    std::vector<uint32_t> intervals;
    uint32_t              lpm_len = 0;

    for (auto c : query) {
        typename sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024>::char_type c_sdsl = c;
        typename sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024>::size_type nl = 0, nr = 0;

        sdsl::backward_search(fm_sdsl, l, r, c_sdsl, nl, nr);

        l = nl;
        r = nr;

        if (l <= r) {
            lpm_len++;
        } else {
            break;
        }
        intervals.push_back(r + 1 - l);
    }

    fsswm::Logger::DebugLog(LOC, "Intervals: " + fsswm::ToString(intervals));
    return lpm_len;
}

}    // namespace

namespace test_fsswm {

using fsswm::Logger;
using fsswm::ToString;
using fsswm::wm::FMIndex;
using fsswm::wm::WaveletMatrix;

void WaveletMatrix_Test() {
    Logger::DebugLog(LOC, "WaveletMatrix_Test...");

    std::vector<uint32_t> data = {3, 4, 0, 0, 7, 6, 1, 2, 2, 0, 1, 6, 5};

    // Build wavelet matrix
    WaveletMatrix wm(data);

    // Check RankCF
    size_t   position = 8;
    uint32_t c        = 6;
    uint32_t count    = wm.RankCF(c, position);
    Logger::DebugLog(LOC, "RankCF(" + std::to_string(c) + "): " + std::to_string(count));

    if (count != 11)
        throw oc::UnitTestFail("count != 11");

    Logger::DebugLog(LOC, "WaveletMatrix_Test - Passed");
}

void FMIndex_Test() {
    Logger::DebugLog(LOC, "FMIndex_Test...");

    std::string dna = "GATTACA";
    // Build FM-index
    FMIndex fm(dna);

    std::string query        = "ATTAA";
    uint32_t    lpm_len      = fm.LongestPrefixMatchLength(query);
    uint32_t    lpm_len_sdsl = ComputeLPMwithSDSL(dna, query);
    Logger::DebugLog(LOC, "lpm_len(" + query + "): " + std::to_string(lpm_len));
    Logger::DebugLog(LOC, "lpm_len_sdsl(" + query + "): " + std::to_string(lpm_len_sdsl));
    if (lpm_len != lpm_len_sdsl)
        throw oc::UnitTestFail("lpm_len != lpm_len_sdsl");

    Logger::DebugLog(LOC, "FMIndex_Test - Passed");
}

}    // namespace test_fsswm
