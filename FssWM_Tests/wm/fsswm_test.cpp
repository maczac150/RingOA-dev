#include "fsswm_test.h"

#include <sdsl/csa_wt.hpp>
#include <sdsl/suffix_arrays.hpp>

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/fmindex.h"
#include "FssWM/wm/fsswm.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/wavelet_matrix.h"

namespace {

const std::string kCurrentPath   = fsswm::GetCurrentDirectory();
const std::string kTestFssWMPath = kCurrentPath + "/data/test/wm/";

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

using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::Channels;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::ShareIo;
using fsswm::sharing::SharePair;
using fsswm::sharing::SharesPair;
using fsswm::wm::FMIndex;
using fsswm::wm::FssWMEvaluator;
using fsswm::wm::FssWMKey;
using fsswm::wm::FssWMKeyGenerator;
using fsswm::wm::FssWMParameters;
using fsswm::wm::KeyIo;
using fsswm::wm::ShareType;
using fsswm::wm::WaveletMatrix;

void FssWM_Offline_Test() {
    Logger::DebugLog(LOC, "FssWM_Offline_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(5, 3),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                  d = params.GetDatabaseBitSize();
        AdditiveSharing2P         ass(d);
        BinarySharing2P           bss(d);
        ReplicatedSharing3P       rss(d);
        BinaryReplicatedSharing3P brss(d);
        FssWMKeyGenerator         gen(params, ass, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<FssWMKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestFssWMPath + "fsswmkey_d" + std::to_string(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        

        // Offline setup
        rss.OfflineSetUp(kTestFssWMPath + "prf");
    }
    Logger::DebugLog(LOC, "FssWM_Offline_Test - Passed");
}

void FssWM_Online_Test() {
    Logger::DebugLog(LOC, "FssWM_Online_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(5, 5),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
    }
    Logger::DebugLog(LOC, "FssWM_Online_Test - Passed");
}

}    // namespace test_fsswm
