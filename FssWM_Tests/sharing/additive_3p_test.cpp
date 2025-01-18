#include "additive_3p_test.h"

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath      = fsswm::utils::GetCurrentDirectory();
const std::string kTestAdditivePath = kCurrentPath + "/data/test/ss3/";

}    // namespace

namespace test_fsswm {

using fsswm::sharing::Channels;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::ShareIo;
using fsswm::sharing::SharePair, fsswm::sharing::SharesPair;
using fsswm::utils::FileIo, fsswm::utils::Logger, fsswm::utils::ToString;
using fsswm::utils::ThreePartyNetworkManager;

const std::vector<uint32_t> kBitsizes = {
    5,
    // 10,
    // 15,
    // 20,
};

void Additive3P_Open_Offline_Test() {
    Logger::DebugLog(LOC, "Additive3P_Open_Offline_Test");

    for (const uint32_t bitsize : kBitsizes) {
        ReplicatedSharing3P rss(bitsize);
        ShareIo             sh_io;

        uint32_t              x     = 10;
        std::vector<uint32_t> x_vec = {1, 2, 3, 4, 5};

        std::array<SharePair, 3>  x_sh     = rss.ShareLocal(x);
        std::array<SharesPair, 3> x_vec_sh = rss.ShareLocal(x_vec);

        x_sh[0].DebugLog(0);
        x_sh[1].DebugLog(1);
        x_sh[2].DebugLog(2);
        x_vec_sh[0].DebugLog(0);
        x_vec_sh[1].DebugLog(1);
        x_vec_sh[2].DebugLog(2);

        const std::string x_path = kTestAdditivePath + "x_n" + std::to_string(bitsize);
        sh_io.SaveShare(x_path + "_0", x_sh[0]);
        sh_io.SaveShare(x_path + "_1", x_sh[1]);
        sh_io.SaveShare(x_path + "_2", x_sh[2]);
        sh_io.SaveShare(x_path + "_vec_0", x_vec_sh[0]);
        sh_io.SaveShare(x_path + "_vec_1", x_vec_sh[1]);
        sh_io.SaveShare(x_path + "_vec_2", x_vec_sh[2]);
    }

    Logger::DebugLog(LOC, "Additive3P_Open_Offline_Test - Passed");
}

void Additive3P_Open_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_Open_Online_Test");

    for (const uint32_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Start network communication
        ThreePartyNetworkManager net_mgr;

        uint32_t              open_x;
        std::vector<uint32_t> open_x_vec;
        const std::string     x_path = kTestAdditivePath + "x_n" + std::to_string(bitsize);

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            SharePair           x_0;
            SharesPair          x_vec_0;
            Channels            chls(0, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_0", x_0);
            sh_io.LoadShare(x_path + "_vec_0", x_vec_0);

            // Open shares
            rss.Open(chls, x_0, open_x);
            rss.Open(chls, x_vec_0, open_x_vec);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            SharePair           x_1;
            SharesPair          x_vec_1;
            Channels            chls(1, chl_prev, chl_next);

            // Load shares
            sh_io.LoadShare(x_path + "_1", x_1);
            sh_io.LoadShare(x_path + "_vec_1", x_vec_1);

            // Open shares
            rss.Open(chls, x_1, open_x);
            rss.Open(chls, x_vec_1, open_x_vec);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            ReplicatedSharing3P rss(bitsize);
            SharePair           x_2;
            SharesPair          x_vec_2;
            Channels            chls(2, chl_prev, chl_next);

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
        if (open_x != 10)
            throw oc::UnitTestFail("Open protocol failed.");
        if (open_x_vec != std::vector<uint32_t>({1, 2, 3, 4, 5}))
            throw oc::UnitTestFail("Open protocol failed.");
    }

    Logger::DebugLog(LOC, "Additive3P_Open_Online_Test - Passed");
}

void Additive3P_EvaluateMult_Offline_Test() {
    Logger::DebugLog(LOC, "Additive3P_Mult_Offline_Test");

    for (const uint32_t bitsize : kBitsizes) {
        ReplicatedSharing3P rss(bitsize);
        rss.OfflineSetUp();
    }
}

void Additive3P_EvaluateMult_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_Mult_Online_Test");

    for (const uint32_t bitsize : kBitsizes) {
        ReplicatedSharing3P rss(bitsize);
    }
}

}    // namespace test_fsswm
