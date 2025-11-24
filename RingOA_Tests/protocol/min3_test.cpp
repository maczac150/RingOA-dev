#include "min3_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/protocol/min3.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/utils/file_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/rng.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace {

const std::string kCurrentPath = ringoa::GetCurrentDirectory();
const std::string kTestEqPath  = kCurrentPath + "/data/test/protocol/";

}    // namespace

namespace test_ringoa {

using ringoa::block;
using ringoa::FileIo;
using ringoa::FormatType;
using ringoa::GlobalRng;
using ringoa::Logger;
using ringoa::Mod2N;
using ringoa::ToString, ringoa::Format;
using ringoa::TwoPartyNetworkManager;
using ringoa::proto::KeyIo;
using ringoa::proto::Min3Evaluator;
using ringoa::proto::Min3Key;
using ringoa::proto::Min3KeyGenerator;
using ringoa::proto::Min3Parameters;
using ringoa::sharing::AdditiveSharing2P;

void Min3_Offline_Test() {
    Logger::DebugLog(LOC, "Min3_Offline_Test...");
    std::vector<Min3Parameters> params_list = {
        Min3Parameters(5),
    };

    for (const Min3Parameters &params : params_list) {
        params.PrintParameters();
        uint64_t          n = params.GetInputBitsize();
        uint64_t          e = params.GetOutputBitsize();
        AdditiveSharing2P ss_in(n);
        AdditiveSharing2P ss_out(e);
        Min3KeyGenerator  gen(params, ss_in, ss_out);
        KeyIo             key_io;
        FileIo            file_io;

        // Generate keys
        std::pair<Min3Key, Min3Key> keys = gen.GenerateKeys();

        // Offline Setup
        gen.OfflineSetUp(1, kTestEqPath);

        // Save keys
        std::string key_path = kTestEqPath + "min3key_n" + ToString(n) + "_e" + ToString(e);
        key_io.SaveKey(key_path + "_0", keys.first);
        key_io.SaveKey(key_path + "_1", keys.second);

        // Generate input
        uint64_t                      x1    = 3;
        uint64_t                      x2    = 5;
        uint64_t                      x3    = 8;
        std::pair<uint64_t, uint64_t> x1_sh = ss_in.Share(x1);
        std::pair<uint64_t, uint64_t> x2_sh = ss_in.Share(x2);
        std::pair<uint64_t, uint64_t> x3_sh = ss_in.Share(x3);
        Logger::DebugLog(LOC, "x1: " + ToString(x1) + ", x2: " + ToString(x2) + ", x3: " + ToString(x3));
        Logger::DebugLog(LOC, "x1_sh: " + ToString(x1_sh.first) + ", " + ToString(x1_sh.second));
        Logger::DebugLog(LOC, "x2_sh: " + ToString(x2_sh.first) + ", " + ToString(x2_sh.second));
        Logger::DebugLog(LOC, "x3_sh: " + ToString(x3_sh.first) + ", " + ToString(x3_sh.second));

        // Save input
        std::string x1_path = kTestEqPath + "x1_n" + ToString(n) + "_e" + ToString(e);
        std::string x2_path = kTestEqPath + "x2_n" + ToString(n) + "_e" + ToString(e);
        std::string x3_path = kTestEqPath + "x3_n" + ToString(n) + "_e" + ToString(e);
        file_io.WriteBinary(x1_path + "_0", x1_sh.first);
        file_io.WriteBinary(x1_path + "_1", x1_sh.second);
        file_io.WriteBinary(x2_path + "_0", x2_sh.first);
        file_io.WriteBinary(x2_path + "_1", x2_sh.second);
        file_io.WriteBinary(x3_path + "_0", x3_sh.first);
        file_io.WriteBinary(x3_path + "_1", x3_sh.second);
    }
    Logger::DebugLog(LOC, "Min3_Offline_Test - Passed");
}

void Min3_Online_Test() {
    Logger::DebugLog(LOC, "Min3_Online_Test...");
    std::vector<Min3Parameters> params_list = {
        Min3Parameters(5),
    };

    for (const Min3Parameters &params : params_list) {
        // params.PrintMin3Parameters();
        uint64_t n = params.GetInputBitsize();
        uint64_t e = params.GetOutputBitsize();
        KeyIo    key_io;
        FileIo   file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("Min3_Online_Test");

        std::string key_path = kTestEqPath + "min3key_n" + ToString(n) + "_e" + ToString(e);
        std::string x1_path  = kTestEqPath + "x1_n" + ToString(n) + "_e" + ToString(e);
        std::string x2_path  = kTestEqPath + "x2_n" + ToString(n) + "_e" + ToString(e);
        std::string x3_path  = kTestEqPath + "x3_n" + ToString(n) + "_e" + ToString(e);

        uint64_t x1_0, x1_1;
        uint64_t x2_0, x2_1;
        uint64_t x3_0, x3_1;
        uint64_t y, y_0, y_1;

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            AdditiveSharing2P ss_in(n);
            AdditiveSharing2P ss_out(e);
            Min3Evaluator     eval(params, ss_in, ss_out);

            // Load keys
            Min3Key key_0(0, params);
            key_io.LoadKey(key_path + "_0", key_0);

            // Load input
            file_io.ReadBinary(x1_path + "_0", x1_0);
            file_io.ReadBinary(x2_path + "_0", x2_0);
            file_io.ReadBinary(x3_path + "_0", x3_0);

            // Online Setup
            eval.OnlineSetUp(0, kTestEqPath);

            // Evaluate
            y_0 = eval.EvaluateSharedInput(chl, key_0, {x1_0, x2_0, x3_0});

            // Send output
            ss_out.Reconst(0, chl, y_0, y_1, y);
            Logger::DebugLog(LOC, "[P0] y: " + ToString(y));
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            AdditiveSharing2P ss_in(n);
            AdditiveSharing2P ss_out(e);
            Min3Evaluator     eval(params, ss_in, ss_out);

            // Load keys
            Min3Key key_1(1, params);
            key_io.LoadKey(key_path + "_1", key_1);

            // Load input
            file_io.ReadBinary(x1_path + "_1", x1_1);
            file_io.ReadBinary(x2_path + "_1", x2_1);
            file_io.ReadBinary(x3_path + "_1", x3_1);

            // Online Setup
            eval.OnlineSetUp(1, kTestEqPath);

            // Evaluate
            y_1 = eval.EvaluateSharedInput(chl, key_1, {x1_1, x2_1, x3_1});

            // Send output
            ss_out.Reconst(1, chl, y_0, y_1, y);
            Logger::DebugLog(LOC, "[P1] y: " + ToString(y));
        };

        net_mgr.AutoConfigure(-1, server_task, client_task);
        net_mgr.WaitForCompletion();

        uint64_t min_val = std::min({3ULL, 5ULL, 8ULL});
        Logger::DebugLog(LOC, "min(3, 5, 8) = " + ToString(min_val));
        if (y != min_val)
            throw oc::UnitTestFail("y is not equal to " + ToString(min_val));
    }
    Logger::DebugLog(LOC, "Min3_Online_Test - Passed");
}

}    // namespace test_ringoa
