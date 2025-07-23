#include "wm_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/plain_wm.h"

namespace test_fsswm {

using fsswm::Logger;
using fsswm::ToString;
using fsswm::wm::CharType;
using fsswm::wm::FMIndex;
using fsswm::wm::WaveletMatrix;

void WaveletMatrix_Test() {
    Logger::DebugLog(LOC, "WaveletMatrix_Test...");

    std::string text = "ACGTACGT";
    Logger::DebugLog(LOC, "Text: " + text);

    WaveletMatrix wm(text, CharType::DNA);

    uint64_t cid     = wm.GetMapper().ToId('G');
    size_t   pos     = 6;    // up to position 6 (exclusive)
    uint64_t rank_cf = wm.RankCF(cid, pos);

    Logger::DebugLog(LOC, "RankCF('G', " + ToString(pos) + ") = " + ToString(rank_cf));

    if (rank_cf != 5) {
        throw osuCrypto::UnitTestFail("Expected RankCF('G', 6) == 5");
    }

    std::vector<uint64_t> data = {3, 4, 0, 0, 7, 6, 1, 2, 2, 0, 1, 6, 5};
    wm                         = WaveletMatrix(data, 3);
    uint64_t target            = 3;
    size_t   position          = 8;
    uint64_t count             = wm.RankCF(target, position);

    Logger::DebugLog(LOC, "RankCF(" + ToString(target) + ", " + ToString(position) + ") = " + ToString(count));

    if (count != 8)
        throw osuCrypto::UnitTestFail("Expected RankCF(3, 8) == 8");

    // kth smallest element in the range [2, 8)
    for (size_t k = 0; k <= 6; ++k) {
        uint64_t kth_smallest = wm.kthSmallest(2, 8, k);
        Logger::DebugLog(LOC, "kthSmallest(2, 8, " + ToString(k) + ") = " + ToString(kth_smallest));
    }

    Logger::DebugLog(LOC, "WaveletMatrix_Test - Passed");
}

void FMIndex_Test() {
    Logger::DebugLog(LOC, "FMIndex_Test...");

    std::string dna   = "GATTACA";
    std::string query = "GATTG";

    // Build FM-index
    FMIndex fm(dna, CharType::DNA);

    // Convert to bit matrix
    std::vector<uint64_t> bit_matrix = fm.ConvertToBitMatrix(query);

    uint64_t lpm_len     = fm.ComputeLPMfromWM(query);
    uint64_t lpm_len_bwt = fm.ComputeLPMfromBWT(query);

    Logger::DebugLog(LOC, "LPM(WM)   = " + ToString(lpm_len));
    Logger::DebugLog(LOC, "LPM(BWT)  = " + ToString(lpm_len_bwt));

    if (lpm_len != lpm_len_bwt)
        throw osuCrypto::UnitTestFail("LPM mismatch: FM = " + ToString(lpm_len) + ", SDSL = " + ToString(lpm_len_bwt));

    std::string text = "ARNDCQILVVFP";
    query            = "DCQPP";

    // Build FM-index
    fm = FMIndex(text, CharType::PROTEIN);

    // Convert to bit matrix
    bit_matrix = fm.ConvertToBitMatrix(query);

    lpm_len     = fm.ComputeLPMfromWM(query);
    lpm_len_bwt = fm.ComputeLPMfromBWT(query);

    Logger::DebugLog(LOC, "LPM(MY)    = " + ToString(lpm_len));
    Logger::DebugLog(LOC, "LPM(SDSL)  = " + ToString(lpm_len_bwt));

    if (lpm_len != lpm_len_bwt)
        throw osuCrypto::UnitTestFail("LPM mismatch: FM = " + ToString(lpm_len) + ", SDSL = " + ToString(lpm_len_bwt));

    Logger::DebugLog(LOC, "FMIndex_Test - Passed");
}

}    // namespace test_fsswm
