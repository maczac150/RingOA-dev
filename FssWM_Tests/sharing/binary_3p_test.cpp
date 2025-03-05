#include "binary_3p_test.h"

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestBinaryPath = kCurrentPath + "/data/test/ss3/";

}    // namespace

namespace test_fsswm {

using fsswm::FileIo, fsswm::Logger, fsswm::ToString;
using fsswm::ThreePartyNetworkManager;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::Channels;
using fsswm::sharing::ShareIo;
using fsswm::sharing::SharePair, fsswm::sharing::SharesPair;

const std::vector<uint32_t> kBitsizes = {
    5,
    // 10,
    // 15,
    // 20,
};

void Binary3P_Offline_Test() {
    Logger::DebugLog(LOC, "Binary3P_Open_Offline_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        BinaryReplicatedSharing3P rss(bitsize);
        ShareIo                   sh_io;

        uint32_t              x     = 5;
        uint32_t              y     = 4;
        std::vector<uint32_t> x_vec = {1, 2, 3, 4, 5};
        std::vector<uint32_t> y_vec = {5, 4, 3, 2, 1};

        std::array<SharePair, 3>  x_sh     = rss.ShareLocal(x);
        std::array<SharePair, 3>  y_sh     = rss.ShareLocal(y);
        std::array<SharesPair, 3> x_vec_sh = rss.ShareLocal(x_vec);
        std::array<SharesPair, 3> y_vec_sh = rss.ShareLocal(y_vec);

        for (uint32_t i = 0; i < fsswm::sharing::kNumParties; ++i) {
            x_sh[i].DebugLog(i, "x");
            y_sh[i].DebugLog(i, "y");
            x_vec_sh[i].DebugLog(i, "x_vec");
            y_vec_sh[i].DebugLog(i, "y_vec");
        }

        const std::string x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + std::to_string(bitsize);
        sh_io.SaveShare(x_path + "_0", x_sh[0]);
        sh_io.SaveShare(x_path + "_1", x_sh[1]);
        sh_io.SaveShare(x_path + "_2", x_sh[2]);
        sh_io.SaveShare(y_path + "_0", y_sh[0]);
        sh_io.SaveShare(y_path + "_1", y_sh[1]);
        sh_io.SaveShare(y_path + "_2", y_sh[2]);
        sh_io.SaveShare(x_path + "_vec_0", x_vec_sh[0]);
        sh_io.SaveShare(x_path + "_vec_1", x_vec_sh[1]);
        sh_io.SaveShare(x_path + "_vec_2", x_vec_sh[2]);
        sh_io.SaveShare(y_path + "_vec_0", y_vec_sh[0]);
        sh_io.SaveShare(y_path + "_vec_1", y_vec_sh[1]);
        sh_io.SaveShare(y_path + "_vec_2", y_vec_sh[2]);

        // Offline setup
        rss.OfflineSetUp(kTestBinaryPath + "prf");
    }

    Logger::DebugLog(LOC, "Binary3P_Open_Offline_Test - Passed");
}

void Binary3P_Open_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_Open_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint32_t              open_x;
        std::vector<uint32_t> open_x_vec;
        const std::string     x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_0;
            SharesPair                x_vec_0;
            Channels                  chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);

            // Open shares
            rss.Open(chls, x_0, open_x);
            rss.Open(chls, x_vec_0, open_x_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_1;
            SharesPair                x_vec_1;
            Channels                  chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);

            // Open shares
            rss.Open(chls, x_1, open_x);
            rss.Open(chls, x_vec_1, open_x_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_2;
            SharesPair                x_vec_2;
            Channels                  chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);

            // Open shares
            rss.Open(chls, x_2, open_x);
            rss.Open(chls, x_vec_2, open_x_vec);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_x: " + std::to_string(open_x));
        Logger::DebugLog(LOC, "open_x_vec: " + ToString(open_x_vec));

        // Validate the opened value
        if (open_x != 5)
            throw oc::UnitTestFail("Open protocol failed.");
        if (open_x_vec != std::vector<uint32_t>({1, 2, 3, 4, 5}))
            throw oc::UnitTestFail("Open protocol failed.");
    }

    Logger::DebugLog(LOC, "Binary3P_Open_Online_Test - Passed");
}

void Binary3P_EvaluateXor_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_Xor_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint32_t              open_z;
        std::vector<uint32_t> open_z_vec;
        const std::string     x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);
        const std::string     y_path = kTestBinaryPath + "y_n" + std::to_string(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_0, y_0, z_0;
            SharesPair                x_vec_0, y_vec_0, z_vec_0;
            Channels                  chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(y_path + "_0", y_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(y_path + "_vec_0", y_vec_0);

            // Evaluate Xor
            rss.EvaluateXor(x_0, y_0, z_0);
            rss.EvaluateXor(x_vec_0, y_vec_0, z_vec_0);

            z_0.DebugLog(0, "z");
            z_vec_0.DebugLog(0, "z_vec");

            // Open shares
            rss.Open(chls, z_0, open_z);
            rss.Open(chls, z_vec_0, open_z_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_1, y_1, z_1;
            SharesPair                x_vec_1, y_vec_1, z_vec_1;
            Channels                  chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(y_path + "_1", y_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(y_path + "_vec_1", y_vec_1);

            // Evaluate Xor
            rss.EvaluateXor(x_1, y_1, z_1);
            rss.EvaluateXor(x_vec_1, y_vec_1, z_vec_1);

            z_1.DebugLog(1, "z");
            z_vec_1.DebugLog(1, "z_vec");

            // Open shares
            rss.Open(chls, z_1, open_z);
            rss.Open(chls, z_vec_1, open_z_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_2, y_2, z_2;
            SharesPair                x_vec_2, y_vec_2, z_vec_2;
            Channels                  chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(y_path + "_2", y_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(y_path + "_vec_2", y_vec_2);

            // Evaluate Xor
            rss.EvaluateXor(x_2, y_2, z_2);
            rss.EvaluateXor(x_vec_2, y_vec_2, z_vec_2);

            z_2.DebugLog(2, "z");
            z_vec_2.DebugLog(2, "z_vec");

            // Open shares
            rss.Open(chls, z_2, open_z);
            rss.Open(chls, z_vec_2, open_z_vec);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_z: " + std::to_string(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened value
        if (open_z != (5 ^ 4))
            throw oc::UnitTestFail("Binary protocol failed.");
        if (open_z_vec != std::vector<uint32_t>({(1 ^ 5), (2 ^ 4), (3 ^ 3), (4 ^ 2), (5 ^ 1)}))
            throw oc::UnitTestFail("Binary protocol failed.");
    }

    Logger::DebugLog(LOC, "Binary3P_Xor_Online_Test - Passed");
}

void Binary3P_EvaluateAnd_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_And_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint32_t              open_z;
        std::vector<uint32_t> open_z_vec;
        const std::string     x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);
        const std::string     y_path = kTestBinaryPath + "y_n" + std::to_string(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_0, y_0, z_0;
            SharesPair                x_vec_0, y_vec_0, z_vec_0;
            Channels                  chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(y_path + "_0", y_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(y_path + "_vec_0", y_vec_0);

            // Setup the PRF keys
            rss.OnlineSetUp(0, kTestBinaryPath + "prf");

            // Evaluate And
            rss.EvaluateAnd(chls, x_0, y_0, z_0);
            rss.EvaluateAnd(chls, x_vec_0, y_vec_0, z_vec_0);

            z_0.DebugLog(0, "z");
            z_vec_0.DebugLog(0, "z_vec");

            // Open shares
            rss.Open(chls, z_0, open_z);
            rss.Open(chls, z_vec_0, open_z_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_1, y_1, z_1;
            SharesPair                x_vec_1, y_vec_1, z_vec_1;
            Channels                  chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(y_path + "_1", y_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(y_path + "_vec_1", y_vec_1);

            // Setup the PRF keys
            rss.OnlineSetUp(1, kTestBinaryPath + "prf");

            // Evaluate And
            rss.EvaluateAnd(chls, x_1, y_1, z_1);
            rss.EvaluateAnd(chls, x_vec_1, y_vec_1, z_vec_1);

            z_1.DebugLog(1, "z");
            z_vec_1.DebugLog(1, "z_vec");

            // Open shares
            rss.Open(chls, z_1, open_z);
            rss.Open(chls, z_vec_1, open_z_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            SharePair                 x_2, y_2, z_2;
            SharesPair                x_vec_2, y_vec_2, z_vec_2;
            Channels                  chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(y_path + "_2", y_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(y_path + "_vec_2", y_vec_2);

            // Setup the PRF keys
            rss.OnlineSetUp(2, kTestBinaryPath + "prf");

            // Evaluate And
            rss.EvaluateAnd(chls, x_2, y_2, z_2);
            rss.EvaluateAnd(chls, x_vec_2, y_vec_2, z_vec_2);

            z_2.DebugLog(2, "z");
            z_vec_2.DebugLog(2, "z_vec");

            // Open shares
            rss.Open(chls, z_2, open_z);
            rss.Open(chls, z_vec_2, open_z_vec);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_z: " + std::to_string(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened value
        if (open_z != (5 & 4))
            throw oc::UnitTestFail("Binary protocol failed.");
        if (open_z_vec != std::vector<uint32_t>({(1 & 5), (2 & 4), (3 & 3), (4 & 2), (5 & 1)}))
            throw oc::UnitTestFail("Binary protocol failed.");
    }

    Logger::DebugLog(LOC, "Binary3P_And_Online_Test - Passed");
}

}    // namespace test_fsswm
