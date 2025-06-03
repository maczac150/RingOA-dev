#include "obliv_select_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/obliv_select.h"

namespace {

const std::string kCurrentPath = fsswm::GetCurrentDirectory();
const std::string kTestOSPath  = kCurrentPath + "/data/test/wm/";

}    // namespace

namespace test_fsswm {

using fsswm::Channels;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString, fsswm::Format;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareBlock;
using fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::RepShareView64, fsswm::sharing::RepShareViewBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::KeyIo;
using fsswm::wm::OblivSelectEvaluator;
using fsswm::wm::OblivSelectKey;
using fsswm::wm::OblivSelectKeyGenerator;
using fsswm::wm::OblivSelectParameters;

void OblivSelect_Binary_Offline_Test() {
    Logger::DebugLog(LOC, "OblivSelect_Binary_Offline_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOSPath + "oskey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::vector<fsswm::block> database(1U << d);
        for (size_t i = 0; i < database.size(); ++i) {
            database[i] = fsswm::MakeBlock(0, i);
        }
        uint64_t index = bss.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database: " + Format(database));
        Logger::DebugLog(LOC, "Index: " + ToString(index));

        std::array<RepShareVecBlock, 3> database_sh = brss.ShareLocal(database);
        std::array<RepShare64, 3>       index_sh    = brss.ShareLocal(index);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " shares: " + database_sh[p].ToString());
        }
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " index share: " + index_sh[p].ToString());
        }

        // Save data
        std::string db_path  = kTestOSPath + "db_d" + ToString(d);
        std::string idx_path = kTestOSPath + "idx_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        file_io.WriteBinary(idx_path, index);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestOSPath + "prf");
    }
    Logger::DebugLog(LOC, "OblivSelect_Binary_Offline_Test - Passed");
}

void OblivSelect_Binary_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OblivSelect_Binary_Online_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d = params.GetParameters().GetInputBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        fsswm::block              result;
        std::string               key_path = kTestOSPath + "oskey_d" + ToString(d);
        std::string               db_path  = kTestOSPath + "db_d" + ToString(d);
        std::string               idx_path = kTestOSPath + "idx_d" + ToString(d);
        std::vector<fsswm::block> database;
        uint64_t                  index;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(idx_path, index);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShareVecBlock database_sh;
            RepShare64       index_sh;
            sh_io.LoadShare(db_path + "_0", database_sh);
            sh_io.LoadShare(idx_path + "_0", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestOSPath + "prf");
            // Evaluate
            RepShareBlock result_sh;
            eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            RepShareVecBlock database_sh;
            RepShare64       index_sh;
            sh_io.LoadShare(db_path + "_1", database_sh);
            sh_io.LoadShare(idx_path + "_1", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestOSPath + "prf");
            // Evaluate
            RepShareBlock result_sh;
            eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            RepShareVecBlock database_sh;
            RepShare64       index_sh;
            sh_io.LoadShare(db_path + "_2", database_sh);
            sh_io.LoadShare(idx_path + "_2", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestOSPath + "prf");
            // Evaluate
            RepShareBlock result_sh;
            eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
            brss.Open(chls, result_sh, result);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + Format(result));

        if (result != database[index])
            throw osuCrypto::UnitTestFail("OblivSelect_Binary_Online_Test failed: result = " + Format(result) +
                                          ", expected = " + Format(database[index]));
    }
    Logger::DebugLog(LOC, "OblivSelect_Binary_Online_Test - Passed");
}

void OblivSelect_Binary_Offline2_Test() {
    Logger::DebugLog(LOC, "OblivSelect_Binary_Offline2_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, fsswm::fss::OutputMode::kAdditive),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOSPath + "oskey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::vector<uint64_t> database1(1U << d), database2(1U << d);
        for (size_t i = 0; i < database1.size(); ++i) {
            database1[i] = i;
            database2[i] = i + 1;
        }
        uint64_t index = bss.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database1: " + ToString(database1));
        Logger::DebugLog(LOC, "Database2: " + ToString(database2));
        Logger::DebugLog(LOC, "Index: " + ToString(index));

        std::array<RepShareVec64, 3> database1_sh = brss.ShareLocal(database1);
        std::array<RepShareVec64, 3> database2_sh = brss.ShareLocal(database2);
        std::array<RepShare64, 3>    index_sh     = brss.ShareLocal(index);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " db1: " + database1_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " db2: " + database2_sh[p].ToString());
        }
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " index share: " + index_sh[p].ToString());
        }

        // Save data
        std::string db1_path = kTestOSPath + "db1_d" + ToString(d);
        std::string db2_path = kTestOSPath + "db2_d" + ToString(d);
        std::string idx_path = kTestOSPath + "idx_d" + ToString(d);

        file_io.WriteBinary(db1_path, database1);
        file_io.WriteBinary(db2_path, database2);
        file_io.WriteBinary(idx_path, index);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db1_path + "_" + ToString(p), database1_sh[p]);
            sh_io.SaveShare(db2_path + "_" + ToString(p), database2_sh[p]);
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestOSPath + "prf");
    }
    Logger::DebugLog(LOC, "OblivSelect_Binary_Offline_Test - Passed");
}

void OblivSelect_Binary_Online2_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OblivSelect_Binary_Online2_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, fsswm::fss::OutputMode::kAdditive),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetParameters().GetInputBitsize();
        uint64_t nu = params.GetParameters().GetTerminateBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint64_t              result1, result2;
        std::string           key_path = kTestOSPath + "oskey_d" + ToString(d);
        std::string           db1_path = kTestOSPath + "db1_d" + ToString(d);
        std::string           db2_path = kTestOSPath + "db2_d" + ToString(d);
        std::string           idx_path = kTestOSPath + "idx_d" + ToString(d);
        std::vector<uint64_t> database1, database2;
        uint64_t              index;
        file_io.ReadBinary(db1_path, database1);
        file_io.ReadBinary(db2_path, database2);
        file_io.ReadBinary(idx_path, index);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShareVec64             database1_sh, database2_sh;
            RepShare64                index_sh;
            std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
            sh_io.LoadShare(db1_path + "_0", database1_sh);
            sh_io.LoadShare(db2_path + "_0", database2_sh);
            sh_io.LoadShare(idx_path + "_0", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestOSPath + "prf");
            // Evaluate
            RepShare64 result1_sh, result2_sh;
            eval.Evaluate_FdeThenDP(chls, key, uv_prev, uv_next, RepShareView64(database1_sh), RepShareView64(database2_sh), index_sh, result1_sh, result2_sh);
            brss.Open(chls, result1_sh, result1);
            brss.Open(chls, result2_sh, result2);
            // RepShare64 result1_sh;
            // eval.Evaluate_FdeThenDP(chls, key, uv_prev, uv_next, RepShareView64(database1_sh), index_sh, result1_sh);
            // brss.Open(chls, result1_sh, result1);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            RepShareVec64             database1_sh, database2_sh;
            RepShare64                index_sh;
            std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
            sh_io.LoadShare(db1_path + "_1", database1_sh);
            sh_io.LoadShare(db2_path + "_1", database2_sh);
            sh_io.LoadShare(idx_path + "_1", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestOSPath + "prf");
            // Evaluate
            RepShare64 result1_sh, result2_sh;
            eval.Evaluate_FdeThenDP(chls, key, uv_prev, uv_next, RepShareView64(database1_sh), RepShareView64(database2_sh), index_sh, result1_sh, result2_sh);
            brss.Open(chls, result1_sh, result1);
            brss.Open(chls, result2_sh, result2);
            // RepShare64 result1_sh;
            // eval.Evaluate_FdeThenDP(chls, key, uv_prev, uv_next, RepShareView64(database1_sh), index_sh, result1_sh);
            // brss.Open(chls, result1_sh, result1);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(d);
            OblivSelectEvaluator      eval(params, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            OblivSelectKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            RepShareVec64             database1_sh, database2_sh;
            RepShare64                index_sh;
            std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
            sh_io.LoadShare(db1_path + "_2", database1_sh);
            sh_io.LoadShare(db2_path + "_2", database2_sh);
            sh_io.LoadShare(idx_path + "_2", index_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestOSPath + "prf");
            // Evaluate
            RepShare64 result1_sh, result2_sh;
            eval.Evaluate_FdeThenDP(chls, key, uv_prev, uv_next, RepShareView64(database1_sh), RepShareView64(database2_sh), index_sh, result1_sh, result2_sh);
            brss.Open(chls, result1_sh, result1);
            brss.Open(chls, result2_sh, result2);
            // RepShare64 result1_sh;
            // eval.Evaluate_FdeThenDP(chls, key, uv_prev, uv_next, RepShareView64(database1_sh), index_sh, result1_sh);
            // brss.Open(chls, result1_sh, result1);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result1: " + ToString(result1) + ", Result2: " + ToString(result2));

        if (result1 != database1[index])
            throw osuCrypto::UnitTestFail("OblivSelect_Binary_Online_Test failed: result = " + ToString(result1) +
                                          ", expected = " + ToString(database1[index]));
        if (result2 != database2[index])
            throw osuCrypto::UnitTestFail("OblivSelect_Binary_Online_Test failed: result = " + ToString(result2) +
                                          ", expected = " + ToString(database2[index]));
    }
    Logger::DebugLog(LOC, "OblivSelect_Binary_Online_Test - Passed");
}

}    // namespace test_fsswm
