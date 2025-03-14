#include "obliv_select_test.h"

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/obliv_select.h"

namespace {

const std::string kCurrentPath = fsswm::GetCurrentDirectory();
const std::string kTestOSPath  = kCurrentPath + "/data/test/wm/";

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
using fsswm::wm::KeyIo;
using fsswm::wm::OblivSelectEvaluator;
using fsswm::wm::OblivSelectKey;
using fsswm::wm::OblivSelectKeyGenerator;
using fsswm::wm::OblivSelectParameters;
using fsswm::wm::ShareType;

void OblivSelect_Additive_Offline_Test() {
    Logger::DebugLog(LOC, "OblivSelect_Additive_Offline_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(5, ShareType::kAdditive),
        // OblivSelectParameters(10),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P       ass(d);
        BinarySharing2P         bss(d);
        ReplicatedSharing3P     rss(d);
        OblivSelectKeyGenerator gen(params, ass, bss);
        FileIo                  file_io;
        ShareIo                 sh_io;
        KeyIo                   key_io;

        // Generate keys
        std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOSPath + "oskey_d" + std::to_string(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::vector<uint32_t> database = fsswm::CreateSequence(0, 1U << d);
        uint32_t              index    = ass.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database: " + ToString(database));
        Logger::DebugLog(LOC, "Index: " + std::to_string(index));

        std::array<SharesPair, 3> database_sh = rss.ShareLocal(database);
        std::array<SharePair, 3>  index_sh    = rss.ShareLocal(index);
        for (uint32_t i = 0; i < 3; ++i) {
            database_sh[i].DebugLog(i, "db");
        }
        for (uint32_t i = 0; i < 3; ++i) {
            index_sh[i].DebugLog(i, "idx");
        }

        // Save data
        std::string db_path  = kTestOSPath + "db_d" + std::to_string(d);
        std::string idx_path = kTestOSPath + "idx_d" + std::to_string(d);

        file_io.WriteToFile(db_path, database);
        file_io.WriteToFile(idx_path, index);
        sh_io.SaveShare(db_path + "_0", database_sh[0]);
        sh_io.SaveShare(db_path + "_1", database_sh[1]);
        sh_io.SaveShare(db_path + "_2", database_sh[2]);
        sh_io.SaveShare(idx_path + "_0", index_sh[0]);
        sh_io.SaveShare(idx_path + "_1", index_sh[1]);
        sh_io.SaveShare(idx_path + "_2", index_sh[2]);

        // Offline setup
        rss.OfflineSetUp(kTestOSPath + "prf");
    }
    Logger::DebugLog(LOC, "OblivSelect_Additive_Offline_Test - Passed");
}

void OblivSelect_Additive_Online_Test(const oc::CLP &cmd) {
    Logger::DebugLog(LOC, "OblivSelect_Additive_Online_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(5, ShareType::kAdditive),
        // OblivSelectParameters(10),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t d = params.GetParameters().GetInputBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint32_t              result;
        std::string           key_path = kTestOSPath + "oskey_d" + std::to_string(d);
        std::string           db_path  = kTestOSPath + "db_d" + std::to_string(d);
        std::string           idx_path = kTestOSPath + "idx_d" + std::to_string(d);
        std::vector<uint32_t> database;
        uint32_t              index;
        file_io.ReadFromFile(db_path, database);
        file_io.ReadFromFile(idx_path, index);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, rss, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            SharesPair database_sh;
            SharePair  index_sh;
            sh_io.LoadShare(db_path + "_0", database_sh);
            sh_io.LoadShare(idx_path + "_0", index_sh);
            // Setup the PRF keys
            rss.OnlineSetUp(0, kTestOSPath + "prf");
            // Evaluate
            SharePair             result_sh;
            std::vector<uint32_t> uv_prev(1U << d), uv_next(1U << d);
            eval.EvaluateAdditive(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
            rss.Open(chls, result_sh, result);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, rss, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            SharesPair database_sh;
            SharePair  index_sh;
            sh_io.LoadShare(db_path + "_1", database_sh);
            sh_io.LoadShare(idx_path + "_1", index_sh);
            // Setup the PRF keys
            rss.OnlineSetUp(1, kTestOSPath + "prf");
            // Evaluate
            SharePair             result_sh;
            std::vector<uint32_t> uv_prev(1U << d), uv_next(1U << d);
            eval.EvaluateAdditive(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
            rss.Open(chls, result_sh, result);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, rss, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            SharesPair database_sh;
            SharePair  index_sh;
            sh_io.LoadShare(db_path + "_2", database_sh);
            sh_io.LoadShare(idx_path + "_2", index_sh);
            // Setup the PRF keys
            rss.OnlineSetUp(2, kTestOSPath + "prf");
            // Evaluate
            SharePair             result_sh;
            std::vector<uint32_t> uv_prev(1U << d), uv_next(1U << d);
            eval.EvaluateAdditive(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
            rss.Open(chls, result_sh, result);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + std::to_string(result));

        if (result != database[index])
            throw oc::UnitTestFail("OblivSelect_Additive_Online_Test failed: result = " + std::to_string(result) +
                                   ", expected = " + std::to_string(database[index]));
    }
    Logger::DebugLog(LOC, "OblivSelect_Additive_Online_Test - Passed");
}

void OblivSelect_Binary_Offline_Test() {
    Logger::DebugLog(LOC, "OblivSelect_Binary_Offline_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, ShareType::kBinary),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                  d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P         ass(d);
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, ass, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOSPath + "oskey_d" + std::to_string(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::vector<uint32_t> database = fsswm::CreateSequence(0, 1U << d);
        uint32_t              index    = ass.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database: " + ToString(database));
        Logger::DebugLog(LOC, "Index: " + std::to_string(index));

        std::array<SharesPair, 3> database_sh = brss.ShareLocal(database);
        std::array<SharePair, 3>  index_sh    = brss.ShareLocal(index);
        for (uint32_t i = 0; i < 3; ++i) {
            database_sh[i].DebugLog(i, "db");
        }
        for (uint32_t i = 0; i < 3; ++i) {
            index_sh[i].DebugLog(i, "idx");
        }

        // Save data
        std::string db_path  = kTestOSPath + "db_d" + std::to_string(d);
        std::string idx_path = kTestOSPath + "idx_d" + std::to_string(d);

        file_io.WriteToFile(db_path, database);
        file_io.WriteToFile(idx_path, index);
        sh_io.SaveShare(db_path + "_0", database_sh[0]);
        sh_io.SaveShare(db_path + "_1", database_sh[1]);
        sh_io.SaveShare(db_path + "_2", database_sh[2]);
        sh_io.SaveShare(idx_path + "_0", index_sh[0]);
        sh_io.SaveShare(idx_path + "_1", index_sh[1]);
        sh_io.SaveShare(idx_path + "_2", index_sh[2]);

        // Offline setup
        brss.OfflineSetUp(kTestOSPath + "prf");
    }
    Logger::DebugLog(LOC, "OblivSelect_Binary_Offline_Test - Passed");
}

void OblivSelect_Binary_Online_Test(const oc::CLP &cmd) {
    Logger::DebugLog(LOC, "OblivSelect_Binary_Online_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, ShareType::kBinary),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t d  = params.GetParameters().GetInputBitsize();
        uint32_t nu = params.GetParameters().GetTerminateBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint32_t              result;
        std::string           key_path = kTestOSPath + "oskey_d" + std::to_string(d);
        std::string           db_path  = kTestOSPath + "db_d" + std::to_string(d);
        std::string           idx_path = kTestOSPath + "idx_d" + std::to_string(d);
        std::vector<uint32_t> database;
        uint32_t              index;
        file_io.ReadFromFile(db_path, database);
        file_io.ReadFromFile(idx_path, index);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, rss, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            SharesPair database_sh;
            SharePair  index_sh;
            sh_io.LoadShare(db_path + "_0", database_sh);
            sh_io.LoadShare(idx_path + "_0", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestOSPath + "prf");
            // Evaluate
            SharePair                 result_sh;
            std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
            eval.EvaluateBinary(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, rss, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            SharesPair database_sh;
            SharePair  index_sh;
            sh_io.LoadShare(db_path + "_1", database_sh);
            sh_io.LoadShare(idx_path + "_1", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestOSPath + "prf");
            // Evaluate
            SharePair                 result_sh;
            std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
            eval.EvaluateBinary(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, rss, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            SharesPair database_sh;
            SharePair  index_sh;
            sh_io.LoadShare(db_path + "_2", database_sh);
            sh_io.LoadShare(idx_path + "_2", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestOSPath + "prf");
            // Evaluate
            SharePair                 result_sh;
            std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
            eval.EvaluateBinary(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + std::to_string(result));

        if (result != database[index])
            throw oc::UnitTestFail("OblivSelect_Binary_Online_Test failed: result = " + std::to_string(result) +
                                   ", expected = " + std::to_string(database[index]));
    }
    Logger::DebugLog(LOC, "OblivSelect_Binary_Online_Test - Passed");
}

}    // namespace test_fsswm
