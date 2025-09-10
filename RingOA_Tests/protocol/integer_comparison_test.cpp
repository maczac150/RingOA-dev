#include "integer_comparison_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/integer_comparison.h"
#include "RingOA/protocol/key_io.h"
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
using ringoa::proto::IntegerComparisonEvaluator;
using ringoa::proto::IntegerComparisonKey;
using ringoa::proto::IntegerComparisonKeyGenerator;
using ringoa::proto::IntegerComparisonParameters;
using ringoa::proto::KeyIo;
using ringoa::sharing::AdditiveSharing2P;

void IntegerComparison_Offline_Test() {
    Logger::DebugLog(LOC, "IntegerComparison_Offline_Test...");
    std::vector<IntegerComparisonParameters> params_list = {
        IntegerComparisonParameters(4, 4),
        // IntegerComparisonParameters(4, 1),
    };

    for (const IntegerComparisonParameters &params : params_list) {
        params.PrintParameters();
        uint64_t                      n = params.GetParameters().GetInputBitsize();
        uint64_t                      e = params.GetParameters().GetOutputBitsize();
        AdditiveSharing2P             ss_in(n);
        AdditiveSharing2P             ss_out(e);
        IntegerComparisonKeyGenerator gen(params, ss_in, ss_out);
        KeyIo                         key_io;
        FileIo                        file_io;

        // Generate keys
        std::pair<IntegerComparisonKey, IntegerComparisonKey> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestEqPath + "ickey_n" + ToString(n) + "_e" + ToString(e);
        key_io.SaveKey(key_path + "_0", keys.first);
        key_io.SaveKey(key_path + "_1", keys.second);

        // Generate input
        std::vector<uint64_t>                                   x1    = ringoa::CreateSequence(0, 1 << n);
        std::vector<uint64_t>                                   x2    = ringoa::CreateSequence(0, 1 << n);
        std::pair<std::vector<uint64_t>, std::vector<uint64_t>> x1_sh = ss_in.Share(x1);
        std::pair<std::vector<uint64_t>, std::vector<uint64_t>> x2_sh = ss_in.Share(x2);
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
    Logger::DebugLog(LOC, "IntegerComparison_Offline_Test - Passed");
}

void IntegerComparison_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "IntegerComparison_Online_Test...");
    std::vector<IntegerComparisonParameters> params_list = {
        IntegerComparisonParameters(4, 4),
        // IntegerComparisonParameters(4, 1),
    };

    for (const IntegerComparisonParameters &params : params_list) {
        // params.PrintIntegerComparisonParameters();
        uint64_t                   n = params.GetParameters().GetInputBitsize();
        uint64_t                   e = params.GetParameters().GetOutputBitsize();
        uint64_t                   N = 1 << n;    // Number of inputs
        AdditiveSharing2P          ss_in(n);
        AdditiveSharing2P          ss_out(e);
        IntegerComparisonEvaluator eval(params, ss_in, ss_out);
        KeyIo                      key_io;
        FileIo                     file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("IntegerComparison_Online_Test");

        std::string key_path = kTestEqPath + "ickey_n" + ToString(n) + "_e" + ToString(e);
        std::string x1_path  = kTestEqPath + "x1_n" + ToString(n) + "_e" + ToString(e);
        std::string x2_path  = kTestEqPath + "x2_n" + ToString(n) + "_e" + ToString(e);

        std::vector<uint64_t> x1(N), x1_0(N), x1_1(N);
        std::vector<uint64_t> x2(N), x2_0(N), x2_1(N);
        std::vector<uint64_t> y(N * N), y_0(N * N), y_1(N * N);

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            // Load keys
            IntegerComparisonKey key_0(0, params);
            key_io.LoadKey(key_path + "_0", key_0);

            // Load input
            file_io.ReadBinary(x1_path + "_0", x1_0);
            file_io.ReadBinary(x2_path + "_0", x2_0);

            // Evaluate
            for (size_t i = 0; i < x1_0.size(); ++i) {
                for (size_t j = 0; j < x2_0.size(); ++j) {
                    // Compare x1_0[i] and x2_0[j]
                    y_0[i * x2_0.size() + j] = eval.EvaluateSharedInput(chl, key_0, x1_0[i], x2_0[j]);
                }
            }

            // Send output
            ss_out.Reconst(0, chl, y_0, y_1, y);

            ss_in.Reconst(0, chl, x1_0, x1_1, x1);
            ss_in.Reconst(0, chl, x2_0, x2_1, x2);
            Logger::DebugLog(LOC, "x1: " + ToString(x1));
            Logger::DebugLog(LOC, "x2: " + ToString(x2));
            for (size_t i = 0; i < x1.size(); ++i) {
                for (size_t j = 0; j < x2.size(); ++j) {
                    int64_t s1 = ringoa::UnsignedToSignedNBits(x1[i], n);
                    int64_t s2 = ringoa::UnsignedToSignedNBits(x2[j], n);

                    bool     signed_condition      = (std::abs(s1) + std::abs(s2)) < static_cast<int64_t>(N / 2);
                    uint64_t signed_compare_result = (s1 >= s2) ? 1 : 0;
                    // bool signed_compare_result = (s1 < s2) ? 1 : 0;
                    bool match = (y[i * x2.size() + j] == signed_compare_result);

                    std::string match_result = signed_condition ? (match ? "OK" : "NG") : "N/A";

                    Logger::DebugLog(LOC,
                                     "x1[" + ToString(i) + "] = " + ToString(x1[i]) + " (" + ToString(s1) + ")" +
                                         ", x2[" + ToString(j) + "] = " + ToString(x2[j]) + " (" + ToString(s2) + ")" +
                                         ", y[" + ToString(i * x2.size() + j) + "] = " + ToString(y[i * x2.size() + j]) +
                                         ", comp: " + ToString(signed_compare_result) +
                                         ", cond: " + ToString(signed_condition) +
                                         ", ? " + match_result);

                    if (match_result == "NG") {
                        Logger::FatalLog(LOC, "IntegerComparison failed at index " + ToString(i) +
                                                  ", x1: " + ToString(x1[i]) + ", x2: " + ToString(x2[j]) +
                                                  ", y: " + ToString(y[i * x2.size() + j]) + ", expected: " + ToString(signed_compare_result));
                    }
                }
            }
            for (size_t i = 0; i < x1.size(); ++i) {
                for (size_t j = 0; j < x2.size(); ++j) {
                    bool unsigned_condition      = (std::abs(int64_t(x1[i]) - int64_t(x2[j])) < static_cast<int64_t>(N / 2));
                    bool unsigned_compare_result = (x1[i] >= x2[j]) ? 1 : 0;
                    // bool        unsigned_compare_result = (x1[i] < x2[j]) ? 1 : 0;
                    bool        unsigned_match  = (y[i * x2.size() + j] == unsigned_compare_result);
                    std::string unsigned_result = unsigned_condition
                                                      ? (unsigned_match ? "OK" : "NG")
                                                      : "N/A";

                    Logger::DebugLog(LOC,
                                     "x1[" + ToString(i) + "] = " + ToString(x1[i]) +
                                         ", x2[" + ToString(j) + "] = " + ToString(x2[j]) +
                                         ", y[" + ToString(i * x2.size() + j) + "] = " + ToString(y[i * x2.size() + j]) +
                                         ", comp: " + ToString(unsigned_compare_result) +
                                         ", cond: " + ToString(unsigned_condition) +
                                         ", ? " + unsigned_result);

                    if (unsigned_result == "NG") {
                        Logger::FatalLog(LOC,
                                         "Unsigned comparison failed at (" +
                                             ToString(i) + "," + ToString(j) + "): " +
                                             "got=" + ToString(y[i * x2.size() + j]) +
                                             ", exp=" + ToString(unsigned_compare_result));
                    }
                }
            }
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            // Load keys
            IntegerComparisonKey key_1(1, params);
            key_io.LoadKey(key_path + "_1", key_1);

            // Load input
            file_io.ReadBinary(x1_path + "_1", x1_1);
            file_io.ReadBinary(x2_path + "_1", x2_1);

            // Evaluate
            for (size_t i = 0; i < x1_1.size(); ++i) {
                for (size_t j = 0; j < x2_1.size(); ++j) {
                    // Compare x1_1[i] and x2_1[j]
                    y_1[i * x2_1.size() + j] = eval.EvaluateSharedInput(chl, key_1, x1_1[i], x2_1[j]);
                }
            }

            // Send output
            ss_out.Reconst(1, chl, y_0, y_1, y);

            ss_in.Reconst(1, chl, x1_0, x1_1, x1);
            ss_in.Reconst(1, chl, x2_0, x2_1, x2);
        };

        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, server_task, client_task);
        net_mgr.WaitForCompletion();
    }
    Logger::DebugLog(LOC, "IntegerComparison_Online_Test - Passed");
}

}    // namespace test_ringoa
