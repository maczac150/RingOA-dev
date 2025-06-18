#include "zt_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/fm_index/zero_test.h"
#include "FssWM/protocol/key_io.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath      = fsswm::GetCurrentDirectory();
const std::string kTestZeroTestPath = kCurrentPath + "/data/test/fmi/";

}    // namespace

namespace test_fsswm {

using fsswm::Channels;
using fsswm::CreateSequence;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString;
using fsswm::fm_index::ZeroTestEvaluator;
using fsswm::fm_index::ZeroTestKey;
using fsswm::fm_index::ZeroTestKeyGenerator;
using fsswm::fm_index::ZeroTestParameters;
using fsswm::proto::KeyIo;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareVec64;
using fsswm::sharing::ShareIo;

void ZeroTest_Binary_Offline_Test() {
    Logger::DebugLog(LOC, "ZeroTest_Binary_Offline_Test...");
    std::vector<ZeroTestParameters> params_list = {
        // ZeroTestParameters(5),
        ZeroTestParameters(10),
        // ZeroTestParameters(15),
        // ZeroTestParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  n = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(n);
        BinaryReplicatedSharing3P brss(n);
        ZeroTestKeyGenerator      gen(params, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<ZeroTestKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestZeroTestPath + "ztkey_n" + ToString(n);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the x
        uint64_t              x     = 10;
        std::vector<uint64_t> x_vec = {0, 10, 20, 30, 0, 512};
        Logger::DebugLog(LOC, "Input: " + ToString(x));
        Logger::DebugLog(LOC, "Input: " + ToString(x_vec));

        std::array<RepShare64, 3>    x_sh     = brss.ShareLocal(x);
        std::array<RepShareVec64, 3> x_vec_sh = brss.ShareLocal(x_vec);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x: " + x_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_vec: " + x_vec_sh[p].ToString());
        }

        // Save data
        std::string x_path     = kTestZeroTestPath + "x_n" + ToString(n);
        std::string x_vec_path = kTestZeroTestPath + "x_vec_n" + ToString(n);

        file_io.WriteBinary(x_path, x);
        file_io.WriteBinary(x_vec_path, x_vec);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(x_path + "_" + ToString(p), x_sh[p]);
            sh_io.SaveShare(x_vec_path + "_" + ToString(p), x_vec_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestZeroTestPath + "prf");
    }
    Logger::DebugLog(LOC, "ZeroTest_Binary_Offline_Test - Passed");
}

void ZeroTest_Binary_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "ZeroTest_Binary_Online_Test...");
    std::vector<ZeroTestParameters> params_list = {
        // ZeroTestParameters(5),
        ZeroTestParameters(10),
        // ZeroTestParameters(15),
        // ZeroTestParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t n = params.GetParameters().GetInputBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        uint64_t              result;
        std::vector<uint64_t> result_vec;
        std::string           key_path   = kTestZeroTestPath + "ztkey_n" + ToString(n);
        std::string           x_path     = kTestZeroTestPath + "x_n" + ToString(n);
        std::string           x_vec_path = kTestZeroTestPath + "x_vec_n" + ToString(n);
        uint64_t              x;
        std::vector<uint64_t> x_vec;
        file_io.ReadBinary(x_path, x);
        file_io.ReadBinary(x_vec_path, x_vec);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, brss);
            Channels                  chls(0, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(0, params);
            key_io.LoadKey(key_path + "_0", key);
            // Load data
            RepShare64 x_sh;
            sh_io.LoadShare(x_path + "_0", x_sh);
            RepShareVec64 x_vec_sh;
            sh_io.LoadShare(x_vec_path + "_0", x_vec_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(0, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare64 result_sh;
            eval.Evaluate(chls, key, x_sh, result_sh);
            brss.Open(chls, result_sh, result);
            RepShareVec64 result_vec_sh(x_vec.size());
            for (size_t i = 0; i < x_vec.size(); ++i) {
                RepShare64 tmp_sh;
                eval.Evaluate(chls, key, x_vec_sh.At(i), tmp_sh);
                result_vec_sh.Set(i, tmp_sh);
            }
            brss.Open(chls, result_vec_sh, result_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, brss);
            Channels                  chls(1, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(1, params);
            key_io.LoadKey(key_path + "_1", key);
            // Load data
            RepShare64 x_sh;
            sh_io.LoadShare(x_path + "_1", x_sh);
            RepShareVec64 x_vec_sh;
            sh_io.LoadShare(x_vec_path + "_1", x_vec_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(1, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare64 result_sh;
            eval.Evaluate(chls, key, x_sh, result_sh);
            brss.Open(chls, result_sh, result);
            RepShareVec64 result_vec_sh(x_vec.size());
            for (size_t i = 0; i < x_vec.size(); ++i) {
                RepShare64 tmp_sh;
                eval.Evaluate(chls, key, x_vec_sh.At(i), tmp_sh);
                result_vec_sh.Set(i, tmp_sh);
            }
            brss.Open(chls, result_vec_sh, result_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P brss(n);
            ZeroTestEvaluator         eval(params, brss);
            Channels                  chls(2, chl_prev, chl_next);

            // Load keys
            ZeroTestKey key(2, params);
            key_io.LoadKey(key_path + "_2", key);
            // Load data
            RepShare64 x_sh;
            sh_io.LoadShare(x_path + "_2", x_sh);
            RepShareVec64 x_vec_sh;
            sh_io.LoadShare(x_vec_path + "_2", x_vec_sh);
            // Setup the PRF keys
            brss.OnlineSetUp(2, kTestZeroTestPath + "prf");
            // Evaluate
            RepShare64 result_sh;
            eval.Evaluate(chls, key, x_sh, result_sh);
            brss.Open(chls, result_sh, result);
            RepShareVec64 result_vec_sh(x_vec.size());
            for (size_t i = 0; i < x_vec.size(); ++i) {
                RepShare64 tmp_sh;
                eval.Evaluate(chls, key, x_vec_sh.At(i), tmp_sh);
                result_vec_sh.Set(i, tmp_sh);
            }
            brss.Open(chls, result_vec_sh, result_vec);
        };

        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + ToString(result));
        Logger::DebugLog(LOC, "Result: " + ToString(result_vec));

        if (result != (x == 0 ? 1 : 0)) {
            throw osuCrypto::UnitTestFail("ZeroTest_Binary_Online_Test failed: result = " + ToString(result) + ", x = " + ToString(x));
        }
        Logger::DebugLog(LOC, "ZeroTest_Binary_Online_Test - Passed");
    }
}

}    // namespace test_fsswm
