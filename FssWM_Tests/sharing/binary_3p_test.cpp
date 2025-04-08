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

using fsswm::FileIo, fsswm::Logger, fsswm::ToString, fsswm::ToStringMat;
using fsswm::ThreePartyNetworkManager;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::Channels;
using fsswm::sharing::RepShare, fsswm::sharing::RepShareVec, fsswm::sharing::RepShareMat;
using fsswm::sharing::ShareIo;
using fsswm::sharing::UIntVec, fsswm::sharing::UIntMat;

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

        uint32_t x     = 5;
        uint32_t y     = 4;
        UIntVec  c     = {0, 31};
        UIntVec  x_vec = {1, 2, 3, 4, 5};
        UIntVec  y_vec = {5, 4, 3, 2, 1};
        UIntMat  x_mat = {{1, 2}, {3, 4}, {5, 6}};
        UIntMat  y_mat = {{6, 5}, {4, 3}, {2, 1}};

        std::array<RepShare, 3>    x_sh     = rss.ShareLocal(x);
        std::array<RepShare, 3>    y_sh     = rss.ShareLocal(y);
        std::array<RepShareVec, 3> c_sh     = rss.ShareLocal(c);
        std::array<RepShareVec, 3> x_vec_sh = rss.ShareLocal(x_vec);
        std::array<RepShareVec, 3> y_vec_sh = rss.ShareLocal(y_vec);
        std::array<RepShareMat, 3> x_mat_sh = rss.ShareLocal(x_mat);
        std::array<RepShareMat, 3> y_mat_sh = rss.ShareLocal(y_mat);

        for (size_t p = 0; p < fsswm::sharing::kNumParties; ++p) {
            x_sh[p].DebugLog(p, "x");
            y_sh[p].DebugLog(p, "y");
            c_sh[p].DebugLog(p, "c");
            x_vec_sh[p].DebugLog(p, "x_vec");
            y_vec_sh[p].DebugLog(p, "y_vec");
            x_mat_sh[p].DebugLog(p, "x_mat");
            y_mat_sh[p].DebugLog(p, "y_mat");
        }

        const std::string x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + std::to_string(bitsize);
        const std::string c_path = kTestBinaryPath + "c_n" + std::to_string(bitsize);
        for (size_t p = 0; p < fsswm::sharing::kNumParties; ++p) {
            sh_io.SaveShare(x_path + "_" + std::to_string(p), x_sh[p]);
            sh_io.SaveShare(y_path + "_" + std::to_string(p), y_sh[p]);
            sh_io.SaveShare(c_path + "_" + std::to_string(p), c_sh[p]);
            sh_io.SaveShare(x_path + "_vec_" + std::to_string(p), x_vec_sh[p]);
            sh_io.SaveShare(y_path + "_vec_" + std::to_string(p), y_vec_sh[p]);
            sh_io.SaveShare(x_path + "_mat_" + std::to_string(p), x_mat_sh[p]);
            sh_io.SaveShare(y_path + "_mat_" + std::to_string(p), y_mat_sh[p]);
        }

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

        uint32_t          open_x;
        UIntVec           open_x_vec;
        UIntMat           open_x_mat;
        const std::string x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            RepShare                  x_0;
            RepShareVec               x_vec_0;
            RepShareMat               x_mat_0;
            Channels                  chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(x_path + "_mat_0", x_mat_0);

            // Open shares
            rss.Open(chls, x_0, open_x);
            rss.Open(chls, x_vec_0, open_x_vec);
            rss.Open(chls, x_mat_0, open_x_mat);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            RepShare                  x_1;
            RepShareVec               x_vec_1;
            RepShareMat               x_mat_1;
            Channels                  chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(x_path + "_mat_1", x_mat_1);

            // Open shares
            rss.Open(chls, x_1, open_x);
            rss.Open(chls, x_vec_1, open_x_vec);
            rss.Open(chls, x_mat_1, open_x_mat);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            RepShare                  x_2;
            RepShareVec               x_vec_2;
            RepShareMat               x_mat_2;
            Channels                  chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(x_path + "_mat_2", x_mat_2);

            // Open shares
            rss.Open(chls, x_2, open_x);
            rss.Open(chls, x_vec_2, open_x_vec);
            rss.Open(chls, x_mat_2, open_x_mat);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_x: " + std::to_string(open_x));
        Logger::DebugLog(LOC, "open_x_vec: " + ToString(open_x_vec));
        Logger::DebugLog(LOC, "open_x_mat: " + ToStringMat(open_x_mat));

        // Validate the opened value
        if (open_x != 5)
            throw oc::UnitTestFail("Open protocol failed.");
        if (open_x_vec != std::vector<uint32_t>({1, 2, 3, 4, 5}))
            throw oc::UnitTestFail("Open protocol failed.");
        if (open_x_mat != std::vector<std::vector<uint32_t>>({{1, 2}, {3, 4}, {5, 6}}))
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
            RepShare                  x_0, y_0, z_0;
            RepShareVec               x_vec_0, y_vec_0, z_vec_0;
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
            RepShare                  x_1, y_1, z_1;
            RepShareVec               x_vec_1, y_vec_1, z_vec_1;
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
            RepShare                  x_2, y_2, z_2;
            RepShareVec               x_vec_2, y_vec_2, z_vec_2;
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
            RepShare                  x_0, y_0, z_0;
            RepShareVec               x_vec_0, y_vec_0, z_vec_0;
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
            RepShare                  x_1, y_1, z_1;
            RepShareVec               x_vec_1, y_vec_1, z_vec_1;
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
            RepShare                  x_2, y_2, z_2;
            RepShareVec               x_vec_2, y_vec_2, z_vec_2;
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

void Binary3P_EvaluateSelect_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_Select_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint32_t          open_z0, open_z1;
        const std::string x_path = kTestBinaryPath + "x_n" + std::to_string(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + std::to_string(bitsize);
        const std::string c_path = kTestBinaryPath + "c_n" + std::to_string(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            RepShare                  x_0, y_0, z0_0, z1_0;
            RepShareVec               c_0;
            Channels                  chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(y_path + "_0", y_0);
            sh_io.LoadShare(c_path + "_0", c_0);

            // Setup the PRF keys
            rss.OnlineSetUp(0, kTestBinaryPath + "prf");

            // Evaluate Select
            rss.EvaluateSelect(chls, x_0, y_0, c_0.At(0), z0_0);
            rss.EvaluateSelect(chls, x_0, y_0, c_0.At(1), z1_0);
            z0_0.DebugLog(0, "z0");
            z1_0.DebugLog(0, "z1");
            // Open shares
            rss.Open(chls, z0_0, open_z0);
            rss.Open(chls, z1_0, open_z1);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            RepShare                  x_1, y_1, z0_1, z1_1;
            RepShareVec               c_1;
            Channels                  chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(y_path + "_1", y_1);
            sh_io.LoadShare(c_path + "_1", c_1);

            // Setup the PRF keys
            rss.OnlineSetUp(1, kTestBinaryPath + "prf");

            // Evaluate Select
            rss.EvaluateSelect(chls, x_1, y_1, c_1.At(0), z0_1);
            rss.EvaluateSelect(chls, x_1, y_1, c_1.At(1), z1_1);

            z0_1.DebugLog(1, "z0");
            z1_1.DebugLog(1, "z1");

            // Open shares
            rss.Open(chls, z0_1, open_z0);
            rss.Open(chls, z1_1, open_z1);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            BinaryReplicatedSharing3P rss(bitsize);
            RepShare                  x_2, y_2, z2_0, z2_1;
            RepShareVec               c_2;
            RepShareVec               x_vec_, y_vec_2, z_vec_2;
            Channels                  chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(y_path + "_2", y_2);
            sh_io.LoadShare(c_path + "_2", c_2);

            // Setup the PRF keys
            rss.OnlineSetUp(2, kTestBinaryPath + "prf");

            // Evaluate Select
            rss.EvaluateSelect(chls, x_2, y_2, c_2.At(0), z2_0);
            rss.EvaluateSelect(chls, x_2, y_2, c_2.At(1), z2_1);

            z2_0.DebugLog(2, "z0");
            z2_1.DebugLog(2, "z1");

            // Open shares
            rss.Open(chls, z2_0, open_z0);
            rss.Open(chls, z2_1, open_z1);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_z0: " + std::to_string(open_z0));
        Logger::DebugLog(LOC, "open_z1: " + std::to_string(open_z1));

        // Validate the opened value
        if (open_z0 != 5 || open_z1 != 4)
            throw oc::UnitTestFail("Binary protocol failed.");
    }

    Logger::DebugLog(LOC, "Binary3P_Select_Online_Test - Passed");
}

}    // namespace test_fsswm
