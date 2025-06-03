#include "binary_2p_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/sharing/binary_2p.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestBinaryPath = kCurrentPath + "/data/test/ss/";

}    // namespace

namespace test_fsswm {

using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ToString;
using fsswm::TwoPartyNetworkManager;
using fsswm::sharing::BinarySharing2P;

const std::vector<uint64_t> kBitsizes = {
    5,
    10,
    15,
    20,
};

void Binary2P_EvaluateXor_Offline_Test() {
    Logger::DebugLog(LOC, "Binary2P_EvaluateXor_Offline_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        BinarySharing2P ss(bitsize);
        FileIo          file_io;

        // Generate input
        uint64_t                x     = 5;
        uint64_t                y     = 4;
        std::array<uint64_t, 2> x_arr = {1, 2};
        std::array<uint64_t, 2> y_arr = {5, 4};
        std::vector<uint64_t>   x_vec = {1, 2, 3, 4, 5};
        std::vector<uint64_t>   y_vec = {5, 4, 3, 2, 1};

        // Generate shares
        std::pair<uint64_t, uint64_t>                               x_sh     = ss.Share(x);
        std::pair<uint64_t, uint64_t>                               y_sh     = ss.Share(y);
        std::pair<std::array<uint64_t, 2>, std::array<uint64_t, 2>> x_arr_sh = ss.Share(x_arr);
        std::pair<std::array<uint64_t, 2>, std::array<uint64_t, 2>> y_arr_sh = ss.Share(y_arr);
        std::pair<std::vector<uint64_t>, std::vector<uint64_t>>     x_vec_sh = ss.Share(x_vec);
        std::pair<std::vector<uint64_t>, std::vector<uint64_t>>     y_vec_sh = ss.Share(y_vec);

        Logger::DebugLog(LOC, "x: " + ToString(x) + ", y: " + ToString(y));
        Logger::DebugLog(LOC, "x_0: " + ToString(x_sh.first) + ", x_1: " + ToString(x_sh.second));
        Logger::DebugLog(LOC, "y_0: " + ToString(y_sh.first) + ", y_1: " + ToString(y_sh.second));
        Logger::DebugLog(LOC, "x_arr: " + ToString(x_arr) + ", y_arr: " + ToString(y_arr));
        Logger::DebugLog(LOC, "x_arr_0: " + ToString(x_arr_sh.first) + ", x_arr_1: " + ToString(x_arr_sh.second));
        Logger::DebugLog(LOC, "y_arr_0: " + ToString(y_arr_sh.first) + ", y_arr_1: " + ToString(y_arr_sh.second));
        Logger::DebugLog(LOC, "x_vec: " + ToString(x_vec) + ", y_vec: " + ToString(y_vec));
        Logger::DebugLog(LOC, "x_vec_0: " + ToString(x_vec_sh.first) + ", x_vec_1: " + ToString(x_vec_sh.second));
        Logger::DebugLog(LOC, "y_vec_0: " + ToString(y_vec_sh.first) + ", y_vec_1: " + ToString(y_vec_sh.second));

        // Save input
        std::string x_path = kTestBinaryPath + "x_n" + ToString(bitsize);
        std::string y_path = kTestBinaryPath + "y_n" + ToString(bitsize);
        file_io.WriteBinary(x_path + "_0", x_sh.first);
        file_io.WriteBinary(x_path + "_1", x_sh.second);
        file_io.WriteBinary(y_path + "_0", y_sh.first);
        file_io.WriteBinary(y_path + "_1", y_sh.second);
        file_io.WriteBinary(x_path + "_arr_0", x_arr_sh.first);
        file_io.WriteBinary(x_path + "_arr_1", x_arr_sh.second);
        file_io.WriteBinary(y_path + "_arr_0", y_arr_sh.first);
        file_io.WriteBinary(y_path + "_arr_1", y_arr_sh.second);
        file_io.WriteBinary(x_path + "_vec_0", x_vec_sh.first);
        file_io.WriteBinary(x_path + "_vec_1", x_vec_sh.second);
        file_io.WriteBinary(y_path + "_vec_0", y_vec_sh.first);
        file_io.WriteBinary(y_path + "_vec_1", y_vec_sh.second);

        // Load input
        uint64_t                x_0, x_1, y_0, y_1;
        std::array<uint64_t, 2> x_arr_0, x_arr_1, y_arr_0, y_arr_1;
        std::vector<uint64_t>   x_vec_0, x_vec_1, y_vec_0, y_vec_1;
        file_io.ReadBinary(x_path + "_0", x_0);
        file_io.ReadBinary(x_path + "_1", x_1);
        file_io.ReadBinary(y_path + "_0", y_0);
        file_io.ReadBinary(y_path + "_1", y_1);
        file_io.ReadBinary(x_path + "_arr_0", x_arr_0);
        file_io.ReadBinary(x_path + "_arr_1", x_arr_1);
        file_io.ReadBinary(y_path + "_arr_0", y_arr_0);
        file_io.ReadBinary(y_path + "_arr_1", y_arr_1);
        file_io.ReadBinary(x_path + "_vec_0", x_vec_0);
        file_io.ReadBinary(x_path + "_vec_1", x_vec_1);
        file_io.ReadBinary(y_path + "_vec_0", y_vec_0);
        file_io.ReadBinary(y_path + "_vec_1", y_vec_1);

        // Reconstruct
        uint64_t                x_rec, y_rec;
        std::array<uint64_t, 2> x_arr_rec, y_arr_rec;
        std::vector<uint64_t>   x_vec_rec, y_vec_rec;
        ss.ReconstLocal(x_0, x_1, x_rec);
        ss.ReconstLocal(y_0, y_1, y_rec);
        ss.ReconstLocal(x_arr_0, x_arr_1, x_arr_rec);
        ss.ReconstLocal(y_arr_0, y_arr_1, y_arr_rec);
        ss.ReconstLocal(x_vec_0, x_vec_1, x_vec_rec);
        ss.ReconstLocal(y_vec_0, y_vec_1, y_vec_rec);

        Logger::DebugLog(LOC, "x_rec: " + ToString(x_rec) + ", y_rec: " + ToString(y_rec));
        Logger::DebugLog(LOC, "x_arr_rec: " + ToString(x_arr_rec) + ", y_arr_rec: " + ToString(y_arr_rec));
        Logger::DebugLog(LOC, "x_vec_rec: " + ToString(x_vec_rec) + ", y_vec_rec: " + ToString(y_vec_rec));
    }

    Logger::DebugLog(LOC, "Binary2P_EvaluateXor_Offline_Test - Passed");
}

void Binary2P_EvaluateXor_Online_Test() {
    Logger::DebugLog(LOC, "Binary2P_EvaluateXor_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        BinarySharing2P ss(bitsize);
        FileIo          file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("Binary2P_EvaluateXor_Online_Test");

        uint64_t                z_0, z_1, z;
        std::array<uint64_t, 2> z_arr_0, z_arr_1, z_arr;
        std::vector<uint64_t>   z_vec_0, z_vec_1, z_vec;
        std::string             x_path = kTestBinaryPath + "x_n" + ToString(bitsize);
        std::string             y_path = kTestBinaryPath + "y_n" + ToString(bitsize);

        // Server task
        auto server_task = [&](osuCrypto::Channel &chl) {
            uint64_t                x_0, y_0;
            std::array<uint64_t, 2> x_arr_0, y_arr_0;
            std::vector<uint64_t>   x_vec_0, y_vec_0;

            // Load input
            file_io.ReadBinary(x_path + "_0", x_0);
            file_io.ReadBinary(y_path + "_0", y_0);
            file_io.ReadBinary(x_path + "_arr_0", x_arr_0);
            file_io.ReadBinary(y_path + "_arr_0", y_arr_0);
            file_io.ReadBinary(x_path + "_vec_0", x_vec_0);
            file_io.ReadBinary(y_path + "_vec_0", y_vec_0);

            // Evaluate Xor
            ss.EvaluateXor(x_0, y_0, z_0);
            ss.EvaluateXor(x_arr_0, y_arr_0, z_arr_0);
            ss.EvaluateXor(x_vec_0, y_vec_0, z_vec_0);

            Logger::DebugLog(LOC, "z_0: " + ToString(z_0));
            Logger::DebugLog(LOC, "z_arr_0: " + ToString(z_arr_0));
            Logger::DebugLog(LOC, "z_vec_0: " + ToString(z_vec_0));

            // Reconstruct
            ss.Reconst(0, chl, z_0, z_1, z);
            ss.Reconst(0, chl, z_arr_0, z_arr_1, z_arr);
            ss.Reconst(0, chl, z_vec_0, z_vec_1, z_vec);
        };

        // Client task
        auto client_task = [&](osuCrypto::Channel &chl) {
            uint64_t                x_1, y_1;
            std::array<uint64_t, 2> x_arr_1, y_arr_1;
            std::vector<uint64_t>   x_vec_1, y_vec_1;

            // Load input
            file_io.ReadBinary(x_path + "_1", x_1);
            file_io.ReadBinary(y_path + "_1", y_1);
            file_io.ReadBinary(x_path + "_arr_1", x_arr_1);
            file_io.ReadBinary(y_path + "_arr_1", y_arr_1);
            file_io.ReadBinary(x_path + "_vec_1", x_vec_1);
            file_io.ReadBinary(y_path + "_vec_1", y_vec_1);

            // Evaluate Xor
            ss.EvaluateXor(x_1, y_1, z_1);
            ss.EvaluateXor(x_arr_1, y_arr_1, z_arr_1);
            ss.EvaluateXor(x_vec_1, y_vec_1, z_vec_1);

            Logger::DebugLog(LOC, "z_1: " + ToString(z_1));
            Logger::DebugLog(LOC, "z_arr_1: " + ToString(z_arr_1));
            Logger::DebugLog(LOC, "z_vec_1: " + ToString(z_vec_1));

            // Reconstruct
            ss.Reconst(1, chl, z_0, z_1, z);
            ss.Reconst(1, chl, z_arr_0, z_arr_1, z_arr);
            ss.Reconst(1, chl, z_vec_0, z_vec_1, z_vec);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, server_task, client_task);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "z: " + ToString(z));
        Logger::DebugLog(LOC, "z_arr: " + ToString(z_arr));
        Logger::DebugLog(LOC, "z_vec: " + ToString(z_vec));

        // Validate the result
        if (z != (5 ^ 4) || z_arr != std::array<uint64_t, 2>{(1 ^ 5), (2 ^ 4)} || z_vec != std::vector<uint64_t>{(1 ^ 5), (2 ^ 4), (3 ^ 3), (4 ^ 2), (5 ^ 1)})
            throw osuCrypto::UnitTestFail("EvaluateXor failed.");

        Logger::DebugLog(LOC, "Binary2P_EvaluateXor_Online_Test - Passed");
    }
}

void Binary2P_EvaluateAnd_Offline_Test() {
    Logger::DebugLog(LOC, "Binary2P_EvaluateAnd_Offline_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        BinarySharing2P ss(bitsize);

        // Generate beaver triples
        std::string triple_path = kTestBinaryPath + "triple_n" + ToString(bitsize);
        ss.OfflineSetUp(3, triple_path);

        ss.OnlineSetUp(0, triple_path);
        ss.PrintTriples();

        ss.OnlineSetUp(1, triple_path);
        ss.PrintTriples();
    }
    Logger::DebugLog(LOC, "Binary2P_EvaluateAnd_Offline_Test - Passed");
}

void Binary2P_EvaluateAnd_Online_Test() {
    Logger::DebugLog(LOC, "Binary2P_EvaluateAnd_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        FileIo file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("Binary2P_EvaluateAnd_Test");

        uint64_t                z_0, z_1, z;
        std::array<uint64_t, 2> z_arr_0, z_arr_1, z_arr;
        std::string             x_path      = kTestBinaryPath + "x_n" + ToString(bitsize);
        std::string             y_path      = kTestBinaryPath + "y_n" + ToString(bitsize);
        std::string             triple_path = kTestBinaryPath + "triple_n" + ToString(bitsize);

        // Server task
        auto server_task = [&](osuCrypto::Channel &chl) {
            BinarySharing2P         ss(bitsize);
            uint64_t                x_0, y_0;
            std::array<uint64_t, 2> x_arr_0, y_arr_0;

            // Load input
            file_io.ReadBinary(x_path + "_0", x_0);
            file_io.ReadBinary(y_path + "_0", y_0);
            file_io.ReadBinary(x_path + "_arr_0", x_arr_0);
            file_io.ReadBinary(y_path + "_arr_0", y_arr_0);

            // Setup additive sharing
            ss.OnlineSetUp(0, triple_path);

            Logger::DebugLog(LOC, "Before: Remaining triples: " + ToString(ss.GetRemainingTripleCount()));

            // Evaluate And
            ss.EvaluateAnd(0, chl, x_0, y_0, z_0);
            ss.EvaluateAnd(0, chl, x_arr_0, y_arr_0, z_arr_0);

            Logger::DebugLog(LOC, "After: Remaining triples: " + ToString(ss.GetRemainingTripleCount()));

            // Reconstruct
            ss.Reconst(0, chl, z_0, z_1, z);
            ss.Reconst(0, chl, z_arr_0, z_arr_1, z_arr);

            Logger::DebugLog(LOC, "z_0: " + ToString(z_0));
            Logger::DebugLog(LOC, "z_arr_0: " + ToString(z_arr_0));
        };

        // Client task

        auto client_task = [&](osuCrypto::Channel &chl) {
            BinarySharing2P         ss(bitsize);
            uint64_t                x_1, y_1;
            std::array<uint64_t, 2> x_arr_1, y_arr_1;

            // Load input
            file_io.ReadBinary(x_path + "_1", x_1);
            file_io.ReadBinary(y_path + "_1", y_1);
            file_io.ReadBinary(x_path + "_arr_1", x_arr_1);
            file_io.ReadBinary(y_path + "_arr_1", y_arr_1);

            // Setup additive sharing
            ss.OnlineSetUp(1, triple_path);

            // Evaluate And
            ss.EvaluateAnd(1, chl, x_1, y_1, z_1);
            ss.EvaluateAnd(1, chl, x_arr_1, y_arr_1, z_arr_1);

            // Reconstruct
            ss.Reconst(1, chl, z_0, z_1, z);
            ss.Reconst(1, chl, z_arr_0, z_arr_1, z_arr);

            Logger::DebugLog(LOC, "z_1: " + ToString(z_1));
            Logger::DebugLog(LOC, "z_arr_1: " + ToString(z_arr_1));
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, server_task, client_task);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "z: " + ToString(z));
        Logger::DebugLog(LOC, "z_arr: " + ToString(z_arr));

        // Validate the result
        if (z != (4 & 5) || z_arr != std::array<uint64_t, 2>{(1 & 5), (2 & 4)})
            throw osuCrypto::UnitTestFail("EvaluateAnd failed.");
    }
    Logger::DebugLog(LOC, "Binary2P_EvaluateAnd_Online_Test - Passed");
}

}    // namespace test_fsswm
