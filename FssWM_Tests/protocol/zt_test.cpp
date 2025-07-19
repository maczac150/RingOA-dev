#include "zt_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/protocol/key_io.h"
#include "FssWM/protocol/zero_test.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath = fsswm::GetCurrentDirectory();
const std::string kTestEqPath  = kCurrentPath + "/data/test/protocol/";

}    // namespace

namespace test_fsswm {

using fsswm::block;
using fsswm::FileIo;
using fsswm::FormatType;
using fsswm::GlobalRng;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::ToString, fsswm::Format;
using fsswm::TwoPartyNetworkManager;
using fsswm::proto::KeyIo;
using fsswm::proto::ZeroTestEvaluator;
using fsswm::proto::ZeroTestKey;
using fsswm::proto::ZeroTestKeyGenerator;
using fsswm::proto::ZeroTestParameters;
using fsswm::sharing::AdditiveSharing2P;

void ZeroTest_Offline_Test() {
    Logger::DebugLog(LOC, "ZeroTest_Offline_Test...");
    std::vector<ZeroTestParameters> params_list = {
        ZeroTestParameters(5, 5),
        ZeroTestParameters(5, 1),
        ZeroTestParameters(10, 10),
        ZeroTestParameters(10, 1),
    };

    for (const ZeroTestParameters &params : params_list) {
        params.PrintParameters();
        uint64_t             n = params.GetParameters().GetInputBitsize();
        uint64_t             e = params.GetParameters().GetOutputBitsize();
        AdditiveSharing2P    ss_in(n);
        AdditiveSharing2P    ss_out(e);
        ZeroTestKeyGenerator gen(params, ss_in, ss_out);
        KeyIo                key_io;
        FileIo               file_io;

        // Generate keys
        std::pair<ZeroTestKey, ZeroTestKey> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestEqPath + "ztkey_n" + ToString(n) + "_e" + ToString(e);
        key_io.SaveKey(key_path + "_0", keys.first);
        key_io.SaveKey(key_path + "_1", keys.second);

        // Generate input
        uint64_t                      x1    = 0;
        uint64_t                      x2    = 5;
        std::pair<uint64_t, uint64_t> x1_sh = ss_in.Share(x1);
        std::pair<uint64_t, uint64_t> x2_sh = ss_in.Share(x2);
        Logger::DebugLog(LOC, "x1: " + ToString(x1) + ", x2: " + ToString(x2));
        Logger::DebugLog(LOC, "x1_sh: " + ToString(x1_sh.first) + ", " + ToString(x1_sh.second));
        Logger::DebugLog(LOC, "x2_sh: " + ToString(x2_sh.first) + ", " + ToString(x2_sh.second));

        // Save input
        std::string x1_path = kTestEqPath + "x1_n" + ToString(n) + "_e" + ToString(e);
        std::string x2_path = kTestEqPath + "x2_n" + ToString(n) + "_e" + ToString(e);
        file_io.WriteBinary(x1_path + "_0", x1_sh.first);
        file_io.WriteBinary(x1_path + "_1", x1_sh.second);
        file_io.WriteBinary(x2_path + "_0", x2_sh.first);
        file_io.WriteBinary(x2_path + "_1", x2_sh.second);
    }
    Logger::DebugLog(LOC, "ZeroTest_Offline_Test - Passed");
}

void ZeroTest_Online_Test() {
    Logger::DebugLog(LOC, "ZeroTest_Online_Test...");
    std::vector<ZeroTestParameters> params_list = {
        ZeroTestParameters(5, 5),
        ZeroTestParameters(5, 1),
        ZeroTestParameters(10, 10),
        ZeroTestParameters(10, 1),
    };

    for (const ZeroTestParameters &params : params_list) {
        // params.PrintZeroTestParameters();
        uint64_t          n = params.GetParameters().GetInputBitsize();
        uint64_t          e = params.GetParameters().GetOutputBitsize();
        AdditiveSharing2P ss_in(n);
        AdditiveSharing2P ss_out(e);
        ZeroTestEvaluator eval(params, ss_in, ss_out);
        KeyIo             key_io;
        FileIo            file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("ZeroTest_Online_Test");

        std::string key_path = kTestEqPath + "ztkey_n" + ToString(n) + "_e" + ToString(e);
        std::string x1_path  = kTestEqPath + "x1_n" + ToString(n) + "_e" + ToString(e);
        std::string x2_path  = kTestEqPath + "x2_n" + ToString(n) + "_e" + ToString(e);

        uint64_t x1_0, x1_1;
        uint64_t x2_0, x2_1;
        uint64_t y1, y1_0, y1_1;
        uint64_t y2, y2_0, y2_1;

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            // Load keys
            ZeroTestKey key_0(0, params);
            key_io.LoadKey(key_path + "_0", key_0);

            // Load input
            file_io.ReadBinary(x1_path + "_0", x1_0);
            file_io.ReadBinary(x2_path + "_0", x2_0);

            // Evaluate
            y1_0 = eval.EvaluateSharedInput(chl, key_0, x1_0);
            y2_0 = eval.EvaluateSharedInput(chl, key_0, x2_0);

            // Send output
            ss_out.Reconst(0, chl, y1_0, y1_1, y1);
            ss_out.Reconst(0, chl, y2_0, y2_1, y2);
            Logger::DebugLog(LOC, "[P0] y1: " + ToString(y1));
            Logger::DebugLog(LOC, "[P0] y2: " + ToString(y2));
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            // Load keys
            ZeroTestKey key_1(1, params);
            key_io.LoadKey(key_path + "_1", key_1);

            // Load input
            file_io.ReadBinary(x1_path + "_1", x1_1);
            file_io.ReadBinary(x2_path + "_1", x2_1);

            // Evaluate
            y1_1 = eval.EvaluateSharedInput(chl, key_1, x1_1);
            y2_1 = eval.EvaluateSharedInput(chl, key_1, x2_1);

            // Send output
            ss_out.Reconst(1, chl, y1_0, y1_1, y1);
            ss_out.Reconst(1, chl, y2_0, y2_1, y2);
            Logger::DebugLog(LOC, "[P1] y1: " + ToString(y1));
            Logger::DebugLog(LOC, "[P1] y2: " + ToString(y2));
        };

        net_mgr.AutoConfigure(-1, server_task, client_task);
        net_mgr.WaitForCompletion();

        if (y1 != 1)
            throw oc::UnitTestFail("y1 is not equal to 1");

        if (y2 != 0)
            throw oc::UnitTestFail("y2 is not equal to 0");
    }
    Logger::DebugLog(LOC, "ZeroTest_Online_Test - Passed");
}

}    // namespace test_fsswm
