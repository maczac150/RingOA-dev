#include "additive_3p_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace {

const std::string kCurrentPath      = ringoa::GetCurrentDirectory();
const std::string kTestAdditivePath = kCurrentPath + "/data/test/ss3/";

}    // namespace

namespace test_ringoa {

using ringoa::Logger, ringoa::ToString, ringoa::ToStringMatrix;
using ringoa::ThreePartyNetworkManager, ringoa::Channels;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareVec64, ringoa::sharing::RepShareMat64;
using ringoa::sharing::ShareIo;

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

        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_sh: " + x_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_sh: " + y_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_vec_sh: " + x_vec_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_vec_sh: " + y_vec_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_flat_sh: " + x_flat_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_flat_sh: " + y_flat_sh[p].ToStringMatrix());
        }

        const std::string x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestAdditivePath + "y_n" + ToString(bitsize);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
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

        uint64_t              open_x;
        std::vector<uint64_t> open_x_vec;
        std::vector<uint64_t> open_x_flat;
        const std::string     x_path = kTestAdditivePath + "x_n" + ToString(bitsize);

        // Define the task for each party
        auto MakeTask = [&](int party_id) {
            return [=, &sh_io, &open_x, &open_x_vec, &open_x_flat](osuCrypto::Channel &chl_next,
                                                                   osuCrypto::Channel &chl_prev) {
                // (1) Set up the sharing object and channels
                ReplicatedSharing3P rss(bitsize);
                Channels            chls(party_id, chl_prev, chl_next);

                // (2) Prepare local variables for each party (different variables for each type)
                RepShare64    x_sh;
                RepShareVec64 x_vec_sh;
                RepShareMat64 x_flat_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(x_path + "_mat_" + ToString(party_id), x_flat_sh);

                // (4) Call Open and write the disclosure results to the same variables for all parties
                rss.Open(chls, x_sh, open_x);
                rss.Open(chls, x_vec_sh, open_x_vec);
                rss.Open(chls, x_flat_sh, open_x_flat);
            };
        };

        // Create tasks for each party
        auto task_p0 = MakeTask(0);
        auto task_p1 = MakeTask(1);
        auto task_p2 = MakeTask(2);

        // Configure network based on party ID and wait for completion
        ThreePartyNetworkManager net_mgr;
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
    Logger::DebugLog(LOC, "Additive3P_EvaluateAdd_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variables to hold the opened results for all parties
        uint64_t              open_z = 0;
        std::vector<uint64_t> open_z_vec;

        const std::string x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestAdditivePath + "y_n" + ToString(bitsize);

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and opened-result references
            return [=, &sh_io, &open_z, &open_z_vec](osuCrypto::Channel &chl_next,
                                                     osuCrypto::Channel &chl_prev) {
                // (1) Set up the sharing object and channels
                ReplicatedSharing3P rss(bitsize);
                Channels            chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for inputs and outputs
                RepShare64    x_sh, y_sh, z_sh;
                RepShareVec64 x_vec_sh, y_vec_sh, z_vec_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(y_path + "_" + ToString(party_id), y_sh);
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(y_path + "_vec_" + ToString(party_id), y_vec_sh);

                // (4) Perform the additive evaluation on both scalar and vector
                rss.EvaluateAdd(x_sh, y_sh, z_sh);
                rss.EvaluateAdd(x_vec_sh, y_vec_sh, z_vec_sh);

                // (5) Log each party's z and z_vec for debugging
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z: " + z_sh.ToString());
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z_vec: " + z_vec_sh.ToString());

                // (6) Open the shares and write the results into the same variables for all parties
                rss.Open(chls, z_sh, open_z);
                rss.Open(chls, z_vec_sh, open_z_vec);
            };
        };

        // Create tasks for parties 0, 1, and 2
        auto task0 = MakeTask(0);
        auto task1 = MakeTask(1);
        auto task2 = MakeTask(2);

        // Configure network based on party ID and wait for completion
        ThreePartyNetworkManager net_mgr;
        net_mgr.AutoConfigure(-1, task0, task1, task2);
        net_mgr.WaitForCompletion();

        // At this point, all parties have the same open_z and open_z_vec
        Logger::DebugLog(LOC, "open_z:     " + ToString(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened values
        if (open_z != 9)
            throw osuCrypto::UnitTestFail("Additive protocol failed: open_z != 9");
        if (open_z_vec != std::vector<uint64_t>({6, 6, 6, 6, 6}))
            throw osuCrypto::UnitTestFail("Additive protocol failed: open_z_vec mismatch");
    }

    Logger::DebugLog(LOC, "Additive3P_EvaluateAdd_Online_Test - Passed");
}

void Additive3P_EvaluateMult_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_EvaluateMult_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variables for opened results (all parties will write into these)
        uint64_t              open_z = 0;
        std::vector<uint64_t> open_z_vec;

        const std::string x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestAdditivePath + "y_n" + ToString(bitsize);

        ThreePartyNetworkManager net_mgr;

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and references to opened-result variables
            return [=, &sh_io, &open_z, &open_z_vec](osuCrypto::Channel &chl_next,
                                                     osuCrypto::Channel &chl_prev) {
                // (1) Set up the replicated-sharing object and channels
                ReplicatedSharing3P rss(bitsize);
                Channels            chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for inputs and outputs
                RepShare64    x_sh, y_sh, z_sh;
                RepShareVec64 x_vec_sh, y_vec_sh, z_vec_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(y_path + "_" + ToString(party_id), y_sh);
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(y_path + "_vec_" + ToString(party_id), y_vec_sh);

                // (4) Setup PRF keys for secure multiplication
                rss.OnlineSetUp(party_id, kTestAdditivePath + "prf");

                // (5) Perform secure multiplication on both scalar and vector shares
                rss.EvaluateMult(chls, x_sh, y_sh, z_sh);
                rss.EvaluateMult(chls, x_vec_sh, y_vec_sh, z_vec_sh);

                // (6) Log each party’s local z shares for debugging
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z: " + z_sh.ToString());
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z_vec: " + z_vec_sh.ToString());

                // (7) Open the shares and write the results into the same variables for all parties
                rss.Open(chls, z_sh, open_z);
                rss.Open(chls, z_vec_sh, open_z_vec);
            };
        };

        // Create tasks for parties 0, 1, and 2
        auto task0 = MakeTask(0);
        auto task1 = MakeTask(1);
        auto task2 = MakeTask(2);

        // Configure network based on party ID (CLI/env) and wait for completion
        net_mgr.AutoConfigure(-1, task0, task1, task2);
        net_mgr.WaitForCompletion();

        // At this point, all parties have the same open_z and open_z_vec
        Logger::DebugLog(LOC, "open_z:     " + ToString(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened values
        if (open_z != 20)
            throw osuCrypto::UnitTestFail("Additive protocol failed: open_z != 20");
        if (open_z_vec != std::vector<uint64_t>({5, 8, 9, 8, 5}))
            throw osuCrypto::UnitTestFail("Additive protocol failed: open_z_vec mismatch");
    }

    Logger::DebugLog(LOC, "Additive3P_EvaluateMult_Online_Test - Passed");
}

void Additive3P_EvaluateInnerProduct_Online_Test() {
    Logger::DebugLog(LOC, "Additive3P_EvaluateInnerProduct_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variable for opened result (all parties will write into this)
        uint64_t open_z = 0;

        const std::string x_path = kTestAdditivePath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestAdditivePath + "y_n" + ToString(bitsize);

        ThreePartyNetworkManager net_mgr;

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and reference to opened-result variable
            return [=, &sh_io, &open_z](osuCrypto::Channel &chl_next,
                                        osuCrypto::Channel &chl_prev) {
                // (1) Set up the replicated-sharing object and channels
                ReplicatedSharing3P rss(bitsize);
                Channels            chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for vector inputs and output
                RepShare64    z_sh;
                RepShareVec64 x_vec_sh, y_vec_sh;

                // (3) Construct file names and load vector shares
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(y_path + "_vec_" + ToString(party_id), y_vec_sh);

                // (4) Setup PRF keys for secure inner product
                rss.OnlineSetUp(party_id, kTestAdditivePath + "prf");

                // (5) Perform secure inner product
                rss.EvaluateInnerProduct(chls, x_vec_sh, y_vec_sh, z_sh);

                // (6) Log each party’s local z share for debugging
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z: " + z_sh.ToString());

                // (7) Open the share and write the result into the same variable for all parties
                rss.Open(chls, z_sh, open_z);
            };
        };

        // Create tasks for parties 0, 1, and 2
        auto task0 = MakeTask(0);
        auto task1 = MakeTask(1);
        auto task2 = MakeTask(2);

        // Configure network based on party ID (CLI/env) and wait for completion
        net_mgr.AutoConfigure(-1, task0, task1, task2);
        net_mgr.WaitForCompletion();

        // At this point, all parties have the same open_z
        Logger::DebugLog(LOC, "open_z: " + ToString(open_z));

        // Validate the opened value
        if (open_z != ringoa::Mod(35, bitsize))
            throw osuCrypto::UnitTestFail(
                "Additive protocol failed: open_z != Mod(35, " + ToString(bitsize) + ")");
    }

    Logger::DebugLog(LOC, "Additive3P_EvaluateInnerProduct_Online_Test - Passed");
}

}    // namespace test_ringoa
