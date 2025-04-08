#include "wm_test.h"

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/plain_wm.h"

namespace test_fsswm {

using fsswm::Logger;
using fsswm::ToString;
using fsswm::wm::FMIndex;
using fsswm::wm::WaveletMatrix;

void WaveletMatrix_Test() {
    Logger::DebugLog(LOC, "WaveletMatrix_Test...");

    std::vector<uint32_t> data = {3, 4, 0, 0, 7, 6, 1, 2, 2, 0, 1, 6, 5};

    // Build wavelet matrix
    WaveletMatrix wm(3, data);

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
    FMIndex fm(3, dna);

    std::string           query        = "ATTAA";
    std::vector<uint32_t> query_bv     = fm.ConvertToBitVector(query);
    uint32_t              lpm_len      = fm.LongestPrefixMatchLength(query);
    uint32_t              lpm_len_sdsl = fm.ComputeLPMwithSDSL(dna, query);
    Logger::DebugLog(LOC, "lpm_len(" + query + "): " + std::to_string(lpm_len));
    Logger::DebugLog(LOC, "lpm_len_sdsl(" + query + "): " + std::to_string(lpm_len_sdsl));
    if (lpm_len != lpm_len_sdsl)
        throw oc::UnitTestFail("lpm_len != lpm_len_sdsl");

    std::vector<std::vector<uint32_t>> rank0 = fm.GetRank0Tables();
    std::vector<std::vector<uint32_t>> rank1 = fm.GetRank1Tables();
    for (size_t i = 0; i < rank0.size(); ++i) {
        Logger::DebugLog(LOC, "rank0[" + std::to_string(i) + "]: " + ToString(rank0[i]));
    }
    for (size_t i = 0; i < rank1.size(); ++i) {
        Logger::DebugLog(LOC, "rank1[" + std::to_string(i) + "]: " + ToString(rank1[i]));
    }
    Logger::DebugLog(LOC, "FMIndex_Test - Passed");
}

}    // namespace test_fsswm
