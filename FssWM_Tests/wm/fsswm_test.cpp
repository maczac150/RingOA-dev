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
#include "FssWM/wm/fsswm.h"
#include "FssWM/wm/key_io.h"

namespace {

const std::string kCurrentPath   = fsswm::GetCurrentDirectory();
const std::string kTestFssWMPath = kCurrentPath + "/data/test/wm/";

std::string GenerateRandomString(size_t length, const std::string &charset = "ATGC") {
    if (charset.empty() || length == 0)
        return "";
    static thread_local std::mt19937_64                       rng(std::random_device{}());    // スレッドごとの高速乱数
    static thread_local std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string                                               result(length, '\0');    // 事前に領域確保
    for (size_t i = 0; i < length; ++i) {
        result[i] = charset[dist(rng)];    // 高速なインデックス選択
    }
    return result;
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
using fsswm::wm::FssWMEvaluator;
using fsswm::wm::FssWMKey;
using fsswm::wm::FssWMKeyGenerator;
using fsswm::wm::FssWMParameters;
using fsswm::wm::KeyIo;
using fsswm::wm::ShareType;

void FssWM_Offline_Test() {
    Logger::DebugLog(LOC, "FssWM_Offline_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(4, 3, ShareType::kAdditive),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                  d  = params.GetDatabaseBitSize();
        uint32_t                  ds = params.GetDatabaseSize();
        uint32_t                  q  = params.GetQuerySize();
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
        std::string database = GenerateRandomString(ds);
        std::string query    = GenerateRandomString(q);
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + query);

        // std::vector<SharesPair> db_sh = gen.GenerateDatabaseShare(database);
        // SharesPair              q_sh  = gen.GenerateQueryShare(query);

        // for (uint32_t i = 0; i < 4; ++i) {
        //     sh_io.SaveShare(kTestFssWMPath + "db_d" + std::to_string(d) + "_s" + std::to_string(i), db_sh[i]);
        // }
        // sh_io.SaveShare(kTestFssWMPath + "query_d" + std::to_string(d), q_sh);

        // Offline setup
        rss.OfflineSetUp(kTestFssWMPath + "prf");
    }
    throw oc::UnitTestSkipped("Not implemented yet");
    Logger::DebugLog(LOC, "FssWM_Offline_Test - Passed");
}

void FssWM_Online_Test() {
    Logger::DebugLog(LOC, "FssWM_Online_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(4, 3, ShareType::kAdditive),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
    }
    throw oc::UnitTestSkipped("Not implemented yet");
    Logger::DebugLog(LOC, "FssWM_Online_Test - Passed");
}

}    // namespace test_fsswm
