#include "fssfmi_test.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/fm_index/fssfmi.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/plain_wm.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestFssFMIPath = kCurrentPath + "/data/test/fmi/";
const uint64_t    kFixedSeed      = 6;

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
using fsswm::CreateSequence;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString;
using fsswm::fm_index::FssFMIEvaluator;
using fsswm::fm_index::FssFMIKey;
using fsswm::fm_index::FssFMIKeyGenerator;
using fsswm::fm_index::FssFMIParameters;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareBlock;
using fsswm::sharing::RepShareMat64, fsswm::sharing::RepShareMatBlock;
using fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::FMIndex;
using fsswm::wm::KeyIo;

void FssFMI_Offline_Test() {
    Logger::DebugLog(LOC, "FssFMI_Offline_Test...");
    std::vector<FssFMIParameters> params_list = {
        FssFMIParameters(10, 10),
        // FssFMIParameters(10),
        // FssFMIParameters(15),
        // FssFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d  = params.GetDatabaseBitSize();
        uint64_t                  ds = params.GetDatabaseSize();
        uint64_t                  qs = params.GetQuerySize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        FssFMIKeyGenerator        gen(params, bss, brss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<FssFMIKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestFssFMIPath + "fssfmikey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::string database = GenerateRandomString(ds - 2);
        FMIndex     fm(database);
        std::string query = GenerateRandomString(qs);
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + query);

        std::array<RepShareMatBlock, 3> db_sh    = gen.GenerateDatabaseShare(fm);
        std::array<RepShareMat64, 3>    query_sh = gen.GenerateQueryShare(fm, query);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToStringMatrix());
        }

        // Save data
        std::string db_path    = kTestFssFMIPath + "db_d" + ToString(d);
        std::string query_path = kTestFssFMIPath + "query_d" + ToString(d);

        file_io.WriteToFile(db_path, database);
        file_io.WriteToFile(query_path, query);

        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestFssFMIPath + "prf");
    }
    Logger::DebugLog(LOC, "FssWM_Offline_Test - Passed");
}

void FssFMI_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "FssFMI_Online_Test...");
    std::vector<FssFMIParameters> params_list = {
        FssFMIParameters(10, 10),
        // FssFMIParameters(10),
        // FssFMIParameters(15),
        // FssFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t qs = params.GetQuerySize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        std::vector<uint64_t> result;
        std::string           key_path   = kTestFssFMIPath + "fssfmikey_d" + ToString(d);
        std::string           db_path    = kTestFssFMIPath + "db_d" + ToString(d);
        std::string           query_path = kTestFssFMIPath + "query_d" + ToString(d);
        std::string           database;
        std::string           query;
        file_io.ReadFromFile(db_path, database);
        file_io.ReadFromFile(query_path, query);

        // Define the tasks for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            FssFMIEvaluator           eval(params, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            FssFMIKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShareMatBlock db_sh;
            RepShareMat64    query_sh;
            sh_io.LoadShare(db_path + "_0", db_sh);
            sh_io.LoadShare(query_path + "_0", query_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestFssFMIPath + "prf");
            // Evaluate
            RepShareVec64 result_sh(qs);
            eval.EvaluateLPM(chls, key, db_sh, query_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            FssFMIEvaluator           eval(params, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            FssFMIKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            RepShareMatBlock db_sh;
            RepShareMat64    query_sh;
            sh_io.LoadShare(db_path + "_1", db_sh);
            sh_io.LoadShare(query_path + "_1", query_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestFssFMIPath + "prf");
            // Evaluate
            RepShareVec64 result_sh(qs);
            eval.EvaluateLPM(chls, key, db_sh, query_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            FssFMIEvaluator           eval(params, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            FssFMIKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            RepShareMatBlock db_sh;
            RepShareMat64    query_sh;
            sh_io.LoadShare(db_path + "_2", db_sh);
            sh_io.LoadShare(query_path + "_2", query_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestFssFMIPath + "prf");
            // Evaluate
            RepShareVec64 result_sh(qs);
            eval.EvaluateLPM(chls, key, db_sh, query_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + ToString(result));

        FMIndex  fmi(database);
        uint64_t expected_result = fmi.ComputeLPMfromWM(query);
        uint64_t match_len       = 0;
        for (size_t i = 0; i < result.size(); ++i) {
            if (result[i] == 0) {
                match_len++;
            }
        }
        if (match_len != expected_result)
            throw osuCrypto::UnitTestFail("FssFMI_Online_Test failed: result = " + ToString(match_len) +
                                          ", expected = " + ToString(expected_result));
    }
    Logger::DebugLog(LOC, "FssFMI_Online_Test - Passed");
}

}    // namespace test_fsswm
