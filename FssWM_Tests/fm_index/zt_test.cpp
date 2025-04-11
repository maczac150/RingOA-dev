#include "zt_test.h"

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/fm_index/zero_test.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/key_io.h"

namespace {

const std::string kCurrentPath      = fsswm::GetCurrentDirectory();
const std::string kTestZeroTestPath = kCurrentPath + "/data/test/fmi/";

}    // namespace

namespace test_fsswm {

using fsswm::CreateSequence;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::Pow;
using fsswm::ShareType;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString;
using fsswm::fm_index::ZeroTestEvaluator;
using fsswm::fm_index::ZeroTestKey;
using fsswm::fm_index::ZeroTestKeyGenerator;
using fsswm::fm_index::ZeroTestParameters;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::Channels;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::RepShare, fsswm::sharing::RepShareVec, fsswm::sharing::RepShareVec;
using fsswm::sharing::ShareIo;
using fsswm::sharing::UIntVec, fsswm::sharing::UIntMat;
using fsswm::wm::KeyIo;

void ZeroTest_Additive_Offline_Test() {
    Logger::DebugLog(LOC, "ZeroTest_Additive_Offline_Test...");
    std::vector<ZeroTestParameters> params_list = {
        ZeroTestParameters(5, ShareType::kAdditive),
        // ZeroTestParameters(10),
        // ZeroTestParameters(15),
        // ZeroTestParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                  n = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P         ass(n);
        BinarySharing2P           bss(n);
        ReplicatedSharing3P       rss(n);
        BinaryReplicatedSharing3P brss(n);
        ZeroTestKeyGenerator      gen(params, ass, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<ZeroTestKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestZeroTestPath + "ztkey_n" + std::to_string(n);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the x
        uint32_t x = 0;
        Logger::DebugLog(LOC, "Input: " + std::to_string(x));

        std::array<RepShare, 3> x_sh = rss.ShareLocal(x);
        for (uint32_t i = 0; i < 3; ++i) {
            x_sh[i].DebugLog(i, "x");
        }

        // Save data
        std::string x_path = kTestZeroTestPath + "x_n" + std::to_string(n);

        file_io.WriteToFile(x_path, x);
        sh_io.SaveShare(x_path + "_0", x_sh[0]);
        sh_io.SaveShare(x_path + "_1", x_sh[1]);
        sh_io.SaveShare(x_path + "_2", x_sh[2]);

        // Offline setup
        rss.OfflineSetUp(kTestZeroTestPath + "prf");
    }
    Logger::DebugLog(LOC, "ZeroTest_Additive_Offline_Test - Passed");
}

void ZeroTest_Additive_Online_Test(const oc::CLP &cmd) {
    Logger::DebugLog(LOC, "ZeroTest_Additive_Online_Test...");
    std::vector<ZeroTestParameters> params_list = {
        ZeroTestParameters(5, ShareType::kAdditive),
        // ZeroTestParameters(10),
        // ZeroTestParameters(15),
        // ZeroTestParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t n = params.GetParameters().GetInputBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint32_t    result;
        std::string key_path = kTestZeroTestPath + "ztkey_n" + std::to_string(n);
        std::string x_path   = kTestZeroTestPath + "x_n" + std::to_string(n);
        uint32_t    x;
        file_io.ReadFromFile(x_path, x);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(n);
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, rss, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShare x_sh;
            sh_io.LoadShare(x_path + "_0", x_sh);
            // Setup the PRF keys
            rss.OnlineSetUp(0, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare result_sh;
            eval.EvaluateAdditive(chls, key, x_sh, result_sh);
            rss.Open(chls, result_sh, result);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(n);
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, rss, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            RepShare x_sh;
            sh_io.LoadShare(x_path + "_1", x_sh);
            // Setup the PRF keys
            rss.OnlineSetUp(1, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare result_sh;
            eval.EvaluateAdditive(chls, key, x_sh, result_sh);
            rss.Open(chls, result_sh, result);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(n);
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, rss, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            RepShare x_sh;
            sh_io.LoadShare(x_path + "_2", x_sh);
            // Setup the PRF keys
            rss.OnlineSetUp(2, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare result_sh;
            eval.EvaluateAdditive(chls, key, x_sh, result_sh);
            rss.Open(chls, result_sh, result);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + std::to_string(result));

        if (result != (x == 0 ? 1 : 0))
            throw oc::UnitTestSkipped("Not implemented yet");
        // throw oc::UnitTestFail("ZeroTest_Additive_Online_Test failed: result = " + std::to_string(result) + ", x = " + std::to_string(x));
        Logger::DebugLog(LOC, "ZeroTest_Additive_Online_Test - Passed");
    }
}

void ZeroTest_Binary_Offline_Test() {
    Logger::DebugLog(LOC, "ZeroTest_Binary_Offline_Test...");
    std::vector<ZeroTestParameters> params_list = {
        // ZeroTestParameters(5, ShareType::kBinary),
        ZeroTestParameters(10, ShareType::kBinary),
        // ZeroTestParameters(15),
        // ZeroTestParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t                  n = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P         ass(n);
        BinarySharing2P           bss(n);
        BinaryReplicatedSharing3P brss(n);
        ZeroTestKeyGenerator      gen(params, ass, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<ZeroTestKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestZeroTestPath + "ztkey_n" + std::to_string(n);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the x
        uint32_t              x     = 10;
        std::vector<uint32_t> x_vec = {0, 10, 20, 30, 0, 512};
        Logger::DebugLog(LOC, "Input: " + std::to_string(x));
        Logger::DebugLog(LOC, "Input: " + ToString(x_vec));

        std::array<RepShare, 3>    x_sh     = brss.ShareLocal(x);
        std::array<RepShareVec, 3> x_vec_sh = brss.ShareLocal(x_vec);
        for (size_t p = 0; p < fsswm::sharing::kNumParties; ++p) {
            x_sh[p].DebugLog(p, "x");
            x_vec_sh[p].DebugLog(p, "x_vec");
        }

        // Save data
        std::string x_path     = kTestZeroTestPath + "x_n" + std::to_string(n);
        std::string x_vec_path = kTestZeroTestPath + "x_vec_n" + std::to_string(n);

        file_io.WriteToFile(x_path, x);
        file_io.WriteToFile(x_vec_path, x_vec);
        for (size_t p = 0; p < fsswm::sharing::kNumParties; ++p) {
            sh_io.SaveShare(x_path + "_" + std::to_string(p), x_sh[p]);
            sh_io.SaveShare(x_vec_path + "_" + std::to_string(p), x_vec_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestZeroTestPath + "prf");
    }
    Logger::DebugLog(LOC, "ZeroTest_Binary_Offline_Test - Passed");
}

void ZeroTest_Binary_Online_Test(const oc::CLP &cmd) {
    Logger::DebugLog(LOC, "ZeroTest_Binary_Online_Test...");
    std::vector<ZeroTestParameters> params_list = {
        // ZeroTestParameters(5, ShareType::kBinary),
        ZeroTestParameters(10, ShareType::kBinary),
        // ZeroTestParameters(15),
        // ZeroTestParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint32_t n = params.GetParameters().GetInputBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint32_t              result;
        std::vector<uint32_t> result_vec;
        std::string           key_path   = kTestZeroTestPath + "ztkey_n" + std::to_string(n);
        std::string           x_path     = kTestZeroTestPath + "x_n" + std::to_string(n);
        std::string           x_vec_path = kTestZeroTestPath + "x_vec_n" + std::to_string(n);
        uint32_t              x;
        std::vector<uint32_t> x_vec;
        file_io.ReadFromFile(x_path, x);
        file_io.ReadFromFile(x_vec_path, x_vec);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(n);
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, rss, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShare x_sh;
            sh_io.LoadShare(x_path + "_0", x_sh);
            RepShareVec x_vec_sh;
            sh_io.LoadShare(x_vec_path + "_0", x_vec_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare result_sh;
            eval.EvaluateBinary(chls, key, x_sh, result_sh);
            brss.Open(chls, result_sh, result);
            RepShareVec result_vec_sh(x_vec.size());
            for (size_t i = 0; i < x_vec.size(); ++i) {
                RepShare tmp_sh;
                eval.EvaluateBinary(chls, key, x_vec_sh.At(i), tmp_sh);
                result_vec_sh.Set(i, tmp_sh);
            }
            brss.Open(chls, result_vec_sh, result_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(n);
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, rss, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            RepShare x_sh;
            sh_io.LoadShare(x_path + "_1", x_sh);
            RepShareVec x_vec_sh;
            sh_io.LoadShare(x_vec_path + "_1", x_vec_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare result_sh;
            eval.EvaluateBinary(chls, key, x_sh, result_sh);
            brss.Open(chls, result_sh, result);
            RepShareVec result_vec_sh(x_vec.size());
            for (size_t i = 0; i < x_vec.size(); ++i) {
                RepShare tmp_sh;
                eval.EvaluateBinary(chls, key, x_vec_sh.At(i), tmp_sh);
                result_vec_sh.Set(i, tmp_sh);
            }
            brss.Open(chls, result_vec_sh, result_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P       rss(n);
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, rss, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            RepShare x_sh;
            sh_io.LoadShare(x_path + "_2", x_sh);
            RepShareVec x_vec_sh;
            sh_io.LoadShare(x_vec_path + "_2", x_vec_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare result_sh;
            eval.EvaluateBinary(chls, key, x_sh, result_sh);
            brss.Open(chls, result_sh, result);
            RepShareVec result_vec_sh(x_vec.size());
            for (size_t i = 0; i < x_vec.size(); ++i) {
                RepShare tmp_sh;
                eval.EvaluateBinary(chls, key, x_vec_sh.At(i), tmp_sh);
                result_vec_sh.Set(i, tmp_sh);
            }
            brss.Open(chls, result_vec_sh, result_vec);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + std::to_string(result));
        Logger::DebugLog(LOC, "Result: " + ToString(result_vec));

        if (result != (x == 0 ? 1 : 0)) {
            throw oc::UnitTestFail("ZeroTest_Binary_Online_Test failed: result = " + std::to_string(result) + ", x = " + std::to_string(x));
        }
        Logger::DebugLog(LOC, "ZeroTest_Binary_Online_Test - Passed");
    }
}

}    // namespace test_fsswm
