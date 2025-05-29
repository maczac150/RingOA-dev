#include "fsswm_test.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

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
#include "FssWM/wm/plain_wm.h"

namespace {

const std::string kCurrentPath   = fsswm::GetCurrentDirectory();
const std::string kTestFssWMPath = kCurrentPath + "/data/test/wm/";
const uint64_t    kFixedSeed     = 6;

std::string GenerateRandomString(size_t length, const std::string &charset = "ATGC") {
    if (charset.empty() || length == 0)
        return "";
    static thread_local std::mt19937_64                       rng(kFixedSeed);
    static thread_local std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string                                               result(length, '\0');
    for (size_t i = 0; i < length; ++i) {
        result[i] = charset[dist(rng)];
    }
    return result;
}

}    // namespace

namespace test_fsswm {

using fsswm::Channels;
using fsswm::FileIo;
using fsswm::Logger;
;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString, fsswm::ToStringFlatMat;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareVec64;
using fsswm::sharing::RepShareBlock, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::RepShareMat64, fsswm::sharing::RepShareView64;
using fsswm::sharing::RepShareMatBlock, fsswm::sharing::RepShareViewBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::FMIndex;
using fsswm::wm::FssWMEvaluator;
using fsswm::wm::FssWMKey;
using fsswm::wm::FssWMKeyGenerator;
using fsswm::wm::FssWMParameters;
using fsswm::wm::KeyIo;

void FssWM_Offline_Test() {
    Logger::DebugLog(LOC, "FssWM_Offline_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(10),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d  = params.GetDatabaseBitSize();
        uint64_t                  ds = params.GetDatabaseSize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        FssWMKeyGenerator         gen(params, bss, brss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<FssWMKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestFssWMPath + "fsswmkey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::string           database = GenerateRandomString(ds);
        FMIndex               fm(database);
        std::vector<uint64_t> query    = {0, 1, 0};
        uint64_t              position = brss.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + ToString(query));
        Logger::DebugLog(LOC, "Position: " + ToString(position));

        std::array<RepShareMatBlock, 3> db_sh       = gen.GenerateDatabaseShare(fm);
        std::array<RepShareVec64, 3>    query_sh    = brss.ShareLocal({0, ds - 1, 0});
        std::array<RepShare64, 3>       position_sh = brss.ShareLocal(position);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " position share: " + position_sh[p].ToString());
        }

        // Save data
        std::string db0_path      = kTestFssWMPath + "db0_d" + ToString(d);
        std::string db1_path      = kTestFssWMPath + "db1_d" + ToString(d);
        std::string query_path    = kTestFssWMPath + "query_d" + ToString(d);
        std::string position_path = kTestFssWMPath + "position_d" + ToString(d);

        file_io.WriteToFile(db0_path, database);
        file_io.WriteToFile(db1_path, database);
        file_io.WriteToFile(query_path, query);
        file_io.WriteToFile(position_path, position);

        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db0_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
            sh_io.SaveShare(position_path + "_" + ToString(p), position_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestFssWMPath + "prf");
    }
    Logger::DebugLog(LOC, "FssWM_Offline_Test - Passed");
}

void FssWM_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "FssWM_Online_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(10),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d = params.GetDatabaseBitSize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint64_t              result;
        std::string           key_path      = kTestFssWMPath + "fsswmkey_d" + ToString(d);
        std::string           db0_path      = kTestFssWMPath + "db0_d" + ToString(d);
        std::string           db1_path      = kTestFssWMPath + "db1_d" + ToString(d);
        std::string           query_path    = kTestFssWMPath + "query_d" + ToString(d);
        std::string           position_path = kTestFssWMPath + "position_d" + ToString(d);
        std::string           database;
        std::vector<uint64_t> query;
        uint64_t              position;
        file_io.ReadFromFile(db0_path, database);
        file_io.ReadFromFile(db1_path, database);
        file_io.ReadFromFile(query_path, query);
        file_io.ReadFromFile(position_path, position);

        // Define the tasks for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            FssWMEvaluator            eval(params, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            FssWMKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShareMatBlock db_sh;
            RepShareVec64    query_sh;
            RepShare64       position_sh;
            sh_io.LoadShare(db0_path + "_0", db_sh);
            sh_io.LoadShare(query_path + "_0", query_sh);
            sh_io.LoadShare(position_path + "_0", position_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestFssWMPath + "prf");
            // Evaluate
            RepShare64 result_sh;
            eval.EvaluateRankCF(chls, key, db_sh, RepShareView64(query_sh), position_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            FssWMEvaluator            eval(params, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            FssWMKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            RepShareMatBlock db_sh;
            RepShareVec64    query_sh;
            RepShare64       position_sh;
            sh_io.LoadShare(db0_path + "_1", db_sh);
            sh_io.LoadShare(query_path + "_1", query_sh);
            sh_io.LoadShare(position_path + "_1", position_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestFssWMPath + "prf");
            // Evaluate
            RepShare64 result_sh;
            eval.EvaluateRankCF(chls, key, db_sh, RepShareView64(query_sh), position_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            FssWMEvaluator            eval(params, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            FssWMKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            RepShareMatBlock db_sh;
            RepShareVec64    query_sh;
            RepShare64       position_sh;
            sh_io.LoadShare(db0_path + "_2", db_sh);
            sh_io.LoadShare(query_path + "_2", query_sh);
            sh_io.LoadShare(position_path + "_2", position_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestFssWMPath + "prf");
            // Evaluate
            RepShare64 result_sh;
            eval.EvaluateRankCF(chls, key, db_sh, RepShareView64(query_sh), position_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + ToString(result));

        FMIndex  fmi(database);
        uint64_t expected_result = fmi.GetWaveletMatrix().RankCF(2, position);
        if (result != expected_result)
            throw osuCrypto::UnitTestFail("FssWM_Online_Test failed: result = " + ToString(result) +
                                          ", expected = " + ToString(expected_result));
    }
    Logger::DebugLog(LOC, "FssWM_Online_Test - Passed");
}

}    // namespace test_fsswm
