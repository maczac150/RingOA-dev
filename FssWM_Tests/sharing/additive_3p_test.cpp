#include "additive_3p_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath      = fsswm::GetCurrentDirectory();
const std::string kTestAdditivePath = kCurrentPath + "/data/test/ss3/";

}    // namespace

namespace test_fsswm {

using fsswm::FileIo, fsswm::Logger, fsswm::ToString, fsswm::ToStringMatrix;
using fsswm::ThreePartyNetworkManager, fsswm::Channels;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareMat64;
using fsswm::sharing::ShareIo;

const std::vector<uint64_t> kBitsizes = {
    5,
    // 10,
    // 15,
    // 20,
};

void Additive3P_Offline_Test() {
    Logger::DebugLog(LOC, "Additive3P_Open_Offline_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ReplicatedSharing3P rss(bitsize);
        ShareIo             sh_io;

        uint64_t              x     = 5;
        uint64_t              y     = 4;
        std::vector<uint64_t> x_vec = {1, 2, 3, 4, 5};
        std::vector<uint64_t> y_vec = {5, 4, 3, 2, 1};
        uint64_t              rows = 2, cols = 3;
        std::vector<uint64_t> x_flat = {1, 2, 3, 4, 5, 6};    // 2 rows, 3 columns
        std::vector<uint64_t> y_flat = {3, 4, 5, 6, 7, 8};    // 2 rows, 3 columns

        std::array<RepShare64, 3>    x_sh      = rss.ShareLocal(x);
        std::array<RepShare64, 3>    y_sh      = rss.ShareLocal(y);
        std::array<RepShareVec64, 3> x_vec_sh  = rss.ShareLocal(x_vec);
        std::array<RepShareVec64, 3> y_vec_sh  = rss.ShareLocal(y_vec);
        std::array<RepShareMat64, 3> x_flat_sh = rss.ShareLocal(x_flat, rows, cols);
        std::array<RepShareMat64, 3> y_flat_sh = rss.ShareLocal(y_flat, rows, cols);

        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_sh: " + x_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_sh: " + y_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_vec_sh: " + x_vec_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_vec_sh: " + y_vec_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_flat_sh: " + x_flat_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_flat_sh: " + y_flat_sh[p].ToStringMatrix());
        }

        const std::string x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestAdditivePath + "y_n" + ToString(bitsize);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(x_path + "_" + ToString(p), x_sh[p]);
            sh_io.SaveShare(y_path + "_" + ToString(p), y_sh[p]);
            sh_io.SaveShare(x_path + "_vec_" + ToString(p), x_vec_sh[p]);
            sh_io.SaveShare(y_path + "_vec_" + ToString(p), y_vec_sh[p]);
            sh_io.SaveShare(x_path + "_mat_" + ToString(p), x_flat_sh[p]);
            sh_io.SaveShare(y_path + "_mat_" + ToString(p), y_flat_sh[p]);
        }

        // Offline setup
        rss.OfflineSetUp(kTestAdditivePath + "prf");
    }

    Logger::DebugLog(LOC, "Additive3P_Open_Offline_Test - Passed");
}

void Additive3P_Open_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_Open_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint64_t              open_x;
        std::vector<uint64_t> open_x_vec;
        std::vector<uint64_t> open_x_flat;
        const std::string     x_path = kTestAdditivePath + "x_n" + ToString(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_0;
            RepShareVec64       x_vec_0;
            RepShareMat64       x_flat_0;
            Channels            chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(x_path + "_mat_0", x_flat_0);

            // Open shares
            rss.Open(chls, x_0, open_x);
            rss.Open(chls, x_vec_0, open_x_vec);
            rss.Open(chls, x_flat_0, open_x_flat);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_1;
            RepShareVec64       x_vec_1;
            RepShareMat64       x_flat_1;
            Channels            chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(x_path + "_mat_1", x_flat_1);

            // Open shares
            rss.Open(chls, x_1, open_x);
            rss.Open(chls, x_vec_1, open_x_vec);
            rss.Open(chls, x_flat_1, open_x_flat);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_2;
            RepShareVec64       x_vec_2;
            RepShareMat64       x_flat_2;
            Channels            chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(x_path + "_mat_2", x_flat_2);

            // Open shares
            rss.Open(chls, x_2, open_x);
            rss.Open(chls, x_vec_2, open_x_vec);
            rss.Open(chls, x_flat_2, open_x_flat);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_x: " + ToString(open_x));
        Logger::DebugLog(LOC, "open_x_vec: " + ToString(open_x_vec));
        Logger::DebugLog(LOC, "open_x_flat: " + ToStringMatrix(open_x_flat, 2, 3));

        // Validate the opened value
        if (open_x != 5)
            throw osuCrypto::UnitTestFail("Open protocol failed.");
        if (open_x_vec != std::vector<uint64_t>({1, 2, 3, 4, 5}))
            throw osuCrypto::UnitTestFail("Open protocol failed.");
        if (open_x_flat != std::vector<uint64_t>({1, 2, 3, 4, 5, 6}))
            throw osuCrypto::UnitTestFail("Open protocol failed.");
    }

    Logger::DebugLog(LOC, "Additive3P_Open_Online_Test - Passed");
}

void Additive3P_EvaluateAdd_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_Add_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint64_t              open_z;
        std::vector<uint64_t> open_z_vec;
        const std::string     x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string     y_path = kTestAdditivePath + "y_n" + ToString(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_0, y_0, z_0;
            RepShareVec64       x_vec_0, y_vec_0, z_vec_0;
            Channels            chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(y_path + "_0", y_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(y_path + "_vec_0", y_vec_0);

            // Evaluate Add
            rss.EvaluateAdd(x_0, y_0, z_0);
            rss.EvaluateAdd(x_vec_0, y_vec_0, z_vec_0);

            Logger::DebugLog(LOC, "Party 0 z: " + z_0.ToString());
            Logger::DebugLog(LOC, "Party 0 z_vec: " + z_vec_0.ToString());

            // Open shares
            rss.Open(chls, z_0, open_z);
            rss.Open(chls, z_vec_0, open_z_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_1, y_1, z_1;
            RepShareVec64       x_vec_1, y_vec_1, z_vec_1;
            Channels            chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(y_path + "_1", y_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(y_path + "_vec_1", y_vec_1);

            // Evaluate Add
            rss.EvaluateAdd(x_1, y_1, z_1);
            rss.EvaluateAdd(x_vec_1, y_vec_1, z_vec_1);

            Logger::DebugLog(LOC, "Party 1 z: " + z_1.ToString());
            Logger::DebugLog(LOC, "Party 1 z_vec: " + z_vec_1.ToString());

            // Open shares
            rss.Open(chls, z_1, open_z);
            rss.Open(chls, z_vec_1, open_z_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_2, y_2, z_2;
            RepShareVec64       x_vec_2, y_vec_2, z_vec_2;
            Channels            chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(y_path + "_2", y_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(y_path + "_vec_2", y_vec_2);

            // Evaluate Add
            rss.EvaluateAdd(x_2, y_2, z_2);
            rss.EvaluateAdd(x_vec_2, y_vec_2, z_vec_2);

            Logger::DebugLog(LOC, "Party 2 z: " + z_2.ToString());
            Logger::DebugLog(LOC, "Party 2 z_vec: " + z_vec_2.ToString());

            // Open shares
            rss.Open(chls, z_2, open_z);
            rss.Open(chls, z_vec_2, open_z_vec);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_z: " + ToString(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened value
        if (open_z != 9)
            throw osuCrypto::UnitTestFail("Additive protocol failed.");
        if (open_z_vec != std::vector<uint64_t>({6, 6, 6, 6, 6}))
            throw osuCrypto::UnitTestFail("Additive protocol failed.");
    }

    Logger::DebugLog(LOC, "Additive3P_Add_Online_Test - Passed");
}

void Additive3P_EvaluateMult_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_Mult_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint64_t              open_z;
        std::vector<uint64_t> open_z_vec;
        const std::string     x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string     y_path = kTestAdditivePath + "y_n" + ToString(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_0, y_0, z_0;
            RepShareVec64       x_vec_0, y_vec_0, z_vec_0;
            Channels            chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(y_path + "_0", y_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(y_path + "_vec_0", y_vec_0);

            // Setup the PRF keys
            rss.OnlineSetUp(0, kTestAdditivePath + "prf");

            // Evaluate Mult
            rss.EvaluateMult(chls, x_0, y_0, z_0);
            rss.EvaluateMult(chls, x_vec_0, y_vec_0, z_vec_0);

            Logger::DebugLog(LOC, "Party 0 z: " + z_0.ToString());
            Logger::DebugLog(LOC, "Party 0 z_vec: " + z_vec_0.ToString());

            // Open shares
            rss.Open(chls, z_0, open_z);
            rss.Open(chls, z_vec_0, open_z_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_1, y_1, z_1;
            RepShareVec64       x_vec_1, y_vec_1, z_vec_1;
            Channels            chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(y_path + "_1", y_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(y_path + "_vec_1", y_vec_1);

            // Setup the PRF keys
            rss.OnlineSetUp(1, kTestAdditivePath + "prf");

            // Evaluate Mult
            rss.EvaluateMult(chls, x_1, y_1, z_1);
            rss.EvaluateMult(chls, x_vec_1, y_vec_1, z_vec_1);

            Logger::DebugLog(LOC, "Party 1 z: " + z_1.ToString());
            Logger::DebugLog(LOC, "Party 1 z_vec: " + z_vec_1.ToString());

            // Open shares
            rss.Open(chls, z_1, open_z);
            rss.Open(chls, z_vec_1, open_z_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          x_2, y_2, z_2;
            RepShareVec64       x_vec_2, y_vec_2, z_vec_2;
            Channels            chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_2", x_2);
            sh_io.LoadShare(y_path + "_2", y_2);
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(y_path + "_vec_2", y_vec_2);

            // Setup the PRF keys
            rss.OnlineSetUp(2, kTestAdditivePath + "prf");

            // Evaluate Mult
            rss.EvaluateMult(chls, x_2, y_2, z_2);
            rss.EvaluateMult(chls, x_vec_2, y_vec_2, z_vec_2);

            Logger::DebugLog(LOC, "Party 2 z: " + z_2.ToString());
            Logger::DebugLog(LOC, "Party 2 z_vec: " + z_vec_2.ToString());

            // Open shares
            rss.Open(chls, z_2, open_z);
            rss.Open(chls, z_vec_2, open_z_vec);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_z: " + ToString(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened value
        if (open_z != 20)
            throw osuCrypto::UnitTestFail("Additive protocol failed.");
        if (open_z_vec != std::vector<uint64_t>({5, 8, 9, 8, 5}))
            throw osuCrypto::UnitTestFail("Additive protocol failed.");
    }

    Logger::DebugLog(LOC, "Additive3P_Mult_Online_Test - Passed");
}

void Additive3P_EvaluateInnerProduct_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_InnerProduct_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        uint64_t          open_z;
        const std::string x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestAdditivePath + "y_n" + ToString(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          z_0;
            RepShareVec64       x_vec_0, y_vec_0;
            Channels            chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);
            sh_io.LoadShare(y_path + "_vec_0", y_vec_0);

            // Setup the PRF keys
            rss.OnlineSetUp(0, kTestAdditivePath + "prf");

            // Evaluate Mult
            rss.EvaluateInnerProduct(chls, x_vec_0, y_vec_0, z_0);

            Logger::DebugLog(LOC, "Party 0 z: " + z_0.ToString());

            // Open shares
            rss.Open(chls, z_0, open_z);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          z_1;
            RepShareVec64       x_vec_1, y_vec_1;
            Channels            chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);
            sh_io.LoadShare(y_path + "_vec_1", y_vec_1);

            // Setup the PRF keys
            rss.OnlineSetUp(1, kTestAdditivePath + "prf");

            // Evaluate Mult
            rss.EvaluateInnerProduct(chls, x_vec_1, y_vec_1, z_1);

            Logger::DebugLog(LOC, "Party 1 z: " + z_1.ToString());

            // Open shares
            rss.Open(chls, z_1, open_z);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            RepShare64          z_2;
            RepShareVec64       x_vec_2, y_vec_2;
            Channels            chls(2, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_vec_2", x_vec_2);
            sh_io.LoadShare(y_path + "_vec_2", y_vec_2);

            // Setup the PRF keys
            rss.OnlineSetUp(2, kTestAdditivePath + "prf");

            // Evaluate Mult
            rss.EvaluateInnerProduct(chls, x_vec_2, y_vec_2, z_2);

            Logger::DebugLog(LOC, "Party 2 z: " + z_2.ToString());

            // Open shares
            rss.Open(chls, z_2, open_z);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "open_z: " + ToString(open_z));

        // Validate the opened value
        if (open_z != fsswm::Mod(35, bitsize))
            throw osuCrypto::UnitTestFail("Additive protocol failed.");
    }
}

}    // namespace test_fsswm
