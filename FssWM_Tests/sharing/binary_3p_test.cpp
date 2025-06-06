#include "binary_3p_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestBinaryPath = kCurrentPath + "/data/test/ss3/";

}    // namespace

namespace test_fsswm {

using fsswm::block;
using fsswm::Format, fsswm::FormatMatrix;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager, fsswm::Channels;
using fsswm::ToString, fsswm::ToStringMatrix;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareMat64;
using fsswm::sharing::RepShareBlock, fsswm::sharing::RepShareVecBlock, fsswm::sharing::RepShareMatBlock;
using fsswm::sharing::ShareIo;

const std::vector<uint64_t> kBitsizes = {
    5,
    // 10,
    // 15,
    // 20,
};

void Binary3P_Offline_Test() {
    Logger::DebugLog(LOC, "Binary3P_Open_Offline_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        BinaryReplicatedSharing3P rss(bitsize);
        ShareIo                   sh_io;

        uint64_t              x     = 5;
        uint64_t              y     = 4;
        std::vector<uint64_t> c     = {0, 31};
        std::vector<uint64_t> x_vec = {1, 2, 3, 4, 5};
        std::vector<uint64_t> y_vec = {5, 4, 3, 2, 1};
        uint64_t              rows = 2, cols = 3;
        std::vector<uint64_t> x_flat     = {1, 2, 3, 4, 5, 6};    // 2 rows, 3 columns
        std::vector<uint64_t> y_flat     = {3, 4, 5, 6, 7, 8};    // 2 rows, 3 columns
        block                 x_blk      = fsswm::MakeBlock(0, 0b1010);
        block                 y_blk      = fsswm::MakeBlock(0, 0b0101);
        std::vector<block>    x_vec_blk  = {fsswm::MakeBlock(0, 0b0001), fsswm::MakeBlock(0, 0b0010), fsswm::MakeBlock(0, 0b0011)};
        std::vector<block>    y_vec_blk  = {fsswm::MakeBlock(0, 0b0100), fsswm::MakeBlock(0, 0b0101), fsswm::MakeBlock(0, 0b0110)};
        std::vector<block>    x_flat_blk = {fsswm::MakeBlock(0, 0b0001), fsswm::MakeBlock(0, 0b0010), fsswm::MakeBlock(0, 0b0011),
                                            fsswm::MakeBlock(0, 0b0100), fsswm::MakeBlock(0, 0b0101), fsswm::MakeBlock(0, 0b0110)};
        std::vector<block>    y_flat_blk = {fsswm::MakeBlock(0, 0b0111), fsswm::MakeBlock(0, 0b1000), fsswm::MakeBlock(0, 0b1001),
                                            fsswm::MakeBlock(0, 0b1010), fsswm::MakeBlock(0, 0b1011), fsswm::MakeBlock(0, 0b1100)};

        std::array<RepShare64, 3>       x_sh          = rss.ShareLocal(x);
        std::array<RepShare64, 3>       y_sh          = rss.ShareLocal(y);
        std::array<RepShareVec64, 3>    c_sh          = rss.ShareLocal(c);
        std::array<RepShareVec64, 3>    x_vec_sh      = rss.ShareLocal(x_vec);
        std::array<RepShareVec64, 3>    y_vec_sh      = rss.ShareLocal(y_vec);
        std::array<RepShareMat64, 3>    x_flat_sh     = rss.ShareLocal(x_flat, rows, cols);
        std::array<RepShareMat64, 3>    y_flat_sh     = rss.ShareLocal(y_flat, rows, cols);
        std::array<RepShareBlock, 3>    x_blk_sh      = rss.ShareLocal(x_blk);
        std::array<RepShareBlock, 3>    y_blk_sh      = rss.ShareLocal(y_blk);
        std::array<RepShareVecBlock, 3> x_vec_blk_sh  = rss.ShareLocal(x_vec_blk);
        std::array<RepShareVecBlock, 3> y_vec_blk_sh  = rss.ShareLocal(y_vec_blk);
        std::array<RepShareMatBlock, 3> x_flat_blk_sh = rss.ShareLocal(x_flat_blk, rows, cols);
        std::array<RepShareMatBlock, 3> y_flat_blk_sh = rss.ShareLocal(y_flat_blk, rows, cols);

        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_sh: " + x_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_sh: " + y_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_vec_sh: " + x_vec_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_vec_sh: " + y_vec_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_flat_sh: " + x_flat_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_flat_sh: " + y_flat_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_blk_sh: " + x_blk_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_blk_sh: " + y_blk_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_vec_blk_sh: " + x_vec_blk_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_vec_blk_sh: " + y_vec_blk_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " x_flat_blk_sh: " + x_flat_blk_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " y_flat_blk_sh: " + y_flat_blk_sh[p].ToStringMatrix());
        }

        const std::string x_path = kTestBinaryPath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + ToString(bitsize);
        const std::string c_path = kTestBinaryPath + "c_n" + ToString(bitsize);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(x_path + "_" + ToString(p), x_sh[p]);
            sh_io.SaveShare(y_path + "_" + ToString(p), y_sh[p]);
            sh_io.SaveShare(c_path + "_" + ToString(p), c_sh[p]);
            sh_io.SaveShare(x_path + "_vec_" + ToString(p), x_vec_sh[p]);
            sh_io.SaveShare(y_path + "_vec_" + ToString(p), y_vec_sh[p]);
            sh_io.SaveShare(x_path + "_flat_" + ToString(p), x_flat_sh[p]);
            sh_io.SaveShare(y_path + "_flat_" + ToString(p), y_flat_sh[p]);
            sh_io.SaveShare(x_path + "_blk_" + ToString(p), x_blk_sh[p]);
            sh_io.SaveShare(y_path + "_blk_" + ToString(p), y_blk_sh[p]);
            sh_io.SaveShare(x_path + "_vec_blk_" + ToString(p), x_vec_blk_sh[p]);
            sh_io.SaveShare(y_path + "_vec_blk_" + ToString(p), y_vec_blk_sh[p]);
            sh_io.SaveShare(x_path + "_flat_blk_" + ToString(p), x_flat_blk_sh[p]);
            sh_io.SaveShare(y_path + "_flat_blk_" + ToString(p), y_flat_blk_sh[p]);
        }

        // Offline setup
        rss.OfflineSetUp(kTestBinaryPath + "prf");
    }

    Logger::DebugLog(LOC, "Binary3P_Open_Offline_Test - Passed");
}

void Binary3P_Open_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_Open_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variables for opened results (all parties will write into these)
        uint64_t              open_x = 0;
        std::vector<uint64_t> open_x_vec;
        std::vector<uint64_t> open_x_flat;
        block                 open_x_blk;
        std::vector<block>    open_x_vec_blk;
        std::vector<block>    open_x_flat_blk;

        const std::string x_path = kTestBinaryPath + "x_n" + ToString(bitsize);

        ThreePartyNetworkManager net_mgr;

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and references to opened-result variables
            return [=, &sh_io,
                    &open_x,
                    &open_x_vec,
                    &open_x_flat,
                    &open_x_blk,
                    &open_x_vec_blk,
                    &open_x_flat_blk](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                // (1) Set up the binary replicated-sharing object and channels
                BinaryReplicatedSharing3P rss(bitsize);
                Channels                  chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for all types
                RepShare64       x_sh;
                RepShareVec64    x_vec_sh;
                RepShareMat64    x_flat_sh;
                RepShareBlock    x_blk_sh;
                RepShareVecBlock x_vec_blk_sh;
                RepShareMatBlock x_flat_blk_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(x_path + "_flat_" + ToString(party_id), x_flat_sh);
                sh_io.LoadShare(x_path + "_blk_" + ToString(party_id), x_blk_sh);
                sh_io.LoadShare(x_path + "_vec_blk_" + ToString(party_id), x_vec_blk_sh);
                sh_io.LoadShare(x_path + "_flat_blk_" + ToString(party_id), x_flat_blk_sh);

                // (4) Open all shares and write the results into the common variables
                rss.Open(chls, x_sh, open_x);
                rss.Open(chls, x_vec_sh, open_x_vec);
                rss.Open(chls, x_flat_sh, open_x_flat);
                rss.Open(chls, x_blk_sh, open_x_blk);
                rss.Open(chls, x_vec_blk_sh, open_x_vec_blk);
                rss.Open(chls, x_flat_blk_sh, open_x_flat_blk);
            };
        };

        // Create tasks for parties 0, 1, and 2
        auto task0 = MakeTask(0);
        auto task1 = MakeTask(1);
        auto task2 = MakeTask(2);

        // Configure network based on party ID (CLI/env) and wait for completion
        net_mgr.AutoConfigure(-1, task0, task1, task2);
        net_mgr.WaitForCompletion();

        // At this point, all parties have the same opened values
        Logger::DebugLog(LOC, "open_x:         " + ToString(open_x));
        Logger::DebugLog(LOC, "open_x_vec:     " + ToString(open_x_vec));
        Logger::DebugLog(LOC, "open_x_flat:    " + ToStringMatrix(open_x_flat, 2, 3));
        Logger::DebugLog(LOC, "open_x_blk:     " + Format(open_x_blk));
        Logger::DebugLog(LOC, "open_x_vec_blk: " + Format(open_x_vec_blk));
        Logger::DebugLog(LOC, "open_x_flat_blk:" + FormatMatrix(open_x_flat_blk, 2, 3));

        // Validate the opened values
        if (open_x != 5)
            throw osuCrypto::UnitTestFail("Open protocol failed: open_x != 5");

        if (open_x_vec != std::vector<uint64_t>({1, 2, 3, 4, 5}))
            throw osuCrypto::UnitTestFail("Open protocol failed: open_x_vec mismatch");

        if (open_x_flat != std::vector<uint64_t>({1, 2, 3, 4, 5, 6}))
            throw osuCrypto::UnitTestFail("Open protocol failed: open_x_flat mismatch");

        if (open_x_blk != fsswm::MakeBlock(0, 0b1010))
            throw osuCrypto::UnitTestFail("Open protocol failed: open_x_blk mismatch");

        if (open_x_vec_blk != std::vector<block>({fsswm::MakeBlock(0, 0b0001),
                                                  fsswm::MakeBlock(0, 0b0010),
                                                  fsswm::MakeBlock(0, 0b0011)}))
            throw osuCrypto::UnitTestFail("Open protocol failed: open_x_vec_blk mismatch");

        if (open_x_flat_blk != std::vector<block>({fsswm::MakeBlock(0, 0b0001),
                                                   fsswm::MakeBlock(0, 0b0010),
                                                   fsswm::MakeBlock(0, 0b0011),
                                                   fsswm::MakeBlock(0, 0b0100),
                                                   fsswm::MakeBlock(0, 0b0101),
                                                   fsswm::MakeBlock(0, 0b0110)}))
            throw osuCrypto::UnitTestFail("Open protocol failed: open_x_flat_blk mismatch");
    }

    Logger::DebugLog(LOC, "Binary3P_Open_Online_Test - Passed");
}

void Binary3P_EvaluateXor_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_EvaluateXor_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variables for opened results (all parties will write into these)
        uint64_t              open_z = 0;
        std::vector<uint64_t> open_z_vec;

        const std::string x_path = kTestBinaryPath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + ToString(bitsize);

        ThreePartyNetworkManager net_mgr;

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and references to opened-result variables
            return [=, &sh_io, &open_z, &open_z_vec](osuCrypto::Channel &chl_next,
                                                     osuCrypto::Channel &chl_prev) {
                // (1) Set up the binary replicated-sharing object and channels
                BinaryReplicatedSharing3P rss(bitsize);
                Channels                  chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for inputs and outputs
                RepShare64    x_sh, y_sh, z_sh;
                RepShareVec64 x_vec_sh, y_vec_sh, z_vec_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(y_path + "_" + ToString(party_id), y_sh);
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(y_path + "_vec_" + ToString(party_id), y_vec_sh);

                // (4) Perform secure XOR on both scalar and vector shares
                rss.EvaluateXor(x_sh, y_sh, z_sh);
                rss.EvaluateXor(x_vec_sh, y_vec_sh, z_vec_sh);

                // (5) Log each party’s local z shares for debugging
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

        // Configure network based on party ID (CLI/env) and wait for completion
        net_mgr.AutoConfigure(-1, task0, task1, task2);
        net_mgr.WaitForCompletion();

        // At this point, all parties have the same open_z and open_z_vec
        Logger::DebugLog(LOC, "open_z:     " + ToString(open_z));
        Logger::DebugLog(LOC, "open_z_vec: " + ToString(open_z_vec));

        // Validate the opened values
        if (open_z != (5 ^ 4))
            throw osuCrypto::UnitTestFail("Binary protocol failed: open_z != (5 ^ 4)");

        if (open_z_vec != std::vector<uint64_t>({(1 ^ 5),
                                                 (2 ^ 4),
                                                 (3 ^ 3),
                                                 (4 ^ 2),
                                                 (5 ^ 1)}))
            throw osuCrypto::UnitTestFail("Binary protocol failed: open_z_vec mismatch");
    }

    Logger::DebugLog(LOC, "Binary3P_EvaluateXor_Online_Test - Passed");
}

void Binary3P_EvaluateAnd_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_EvaluateAnd_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variables for opened results (all parties will write into these)
        uint64_t              open_z = 0;
        std::vector<uint64_t> open_z_vec;

        const std::string x_path = kTestBinaryPath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + ToString(bitsize);

        ThreePartyNetworkManager net_mgr;

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and references to opened-result variables
            return [=, &sh_io, &open_z, &open_z_vec](osuCrypto::Channel &chl_next,
                                                     osuCrypto::Channel &chl_prev) {
                // (1) Set up the binary replicated-sharing object and channels
                BinaryReplicatedSharing3P rss(bitsize);
                Channels                  chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for inputs and outputs
                RepShare64    x_sh, y_sh, z_sh;
                RepShareVec64 x_vec_sh, y_vec_sh, z_vec_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(y_path + "_" + ToString(party_id), y_sh);
                sh_io.LoadShare(x_path + "_vec_" + ToString(party_id), x_vec_sh);
                sh_io.LoadShare(y_path + "_vec_" + ToString(party_id), y_vec_sh);

                // (4) Setup PRF keys for secure AND
                rss.OnlineSetUp(party_id, kTestBinaryPath + "prf");

                // (5) Perform secure AND on both scalar and vector shares
                rss.EvaluateAnd(chls, x_sh, y_sh, z_sh);
                rss.EvaluateAnd(chls, x_vec_sh, y_vec_sh, z_vec_sh);

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
        if (open_z != (5 & 4))
            throw osuCrypto::UnitTestFail("Binary protocol failed: open_z != (5 & 4)");

        if (open_z_vec != std::vector<uint64_t>({(1 & 5),
                                                 (2 & 4),
                                                 (3 & 3),
                                                 (4 & 2),
                                                 (5 & 1)}))
            throw osuCrypto::UnitTestFail("Binary protocol failed: open_z_vec mismatch");
    }

    Logger::DebugLog(LOC, "Binary3P_EvaluateAnd_Online_Test - Passed");
}

void Binary3P_EvaluateSelect_Online_Test() {
    Logger::DebugLog(LOC, "Binary3P_EvaluateSelect_Online_Test...");

    for (const uint64_t bitsize : kBitsizes) {
        ShareIo sh_io;

        // Variables for opened results (all parties will write into these)
        uint64_t open_z0 = 0;
        uint64_t open_z1 = 0;

        const std::string x_path = kTestBinaryPath + "x_n" + ToString(bitsize);
        const std::string y_path = kTestBinaryPath + "y_n" + ToString(bitsize);
        const std::string c_path = kTestBinaryPath + "c_n" + ToString(bitsize);

        ThreePartyNetworkManager net_mgr;

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture bitsize, paths, sh_io, and references to opened-result variables
            return [=, &sh_io, &open_z0, &open_z1](osuCrypto::Channel &chl_next,
                                                   osuCrypto::Channel &chl_prev) {
                // (1) Set up the binary replicated-sharing object and channels
                BinaryReplicatedSharing3P rss(bitsize);
                Channels                  chls(party_id, chl_prev, chl_next);

                // (2) Prepare local share variables for inputs, conditions, and outputs
                RepShare64    x_sh, y_sh, z0_sh, z1_sh;
                RepShareVec64 c_sh;

                // (3) Construct file names and load shares
                sh_io.LoadShare(x_path + "_" + ToString(party_id), x_sh);
                sh_io.LoadShare(y_path + "_" + ToString(party_id), y_sh);
                sh_io.LoadShare(c_path + "_" + ToString(party_id), c_sh);

                // (4) Setup PRF keys for secure Select
                rss.OnlineSetUp(party_id, kTestBinaryPath + "prf");

                // (5) Perform secure Select: two outputs based on two bits of c_sh
                rss.EvaluateSelect(chls, x_sh, y_sh, c_sh.At(0), z0_sh);
                rss.EvaluateSelect(chls, x_sh, y_sh, c_sh.At(1), z1_sh);

                // (6) Log each party’s local z0 and z1 shares for debugging
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z0: " + z0_sh.ToString());
                Logger::DebugLog(LOC, "Party " + ToString(party_id) +
                                          " z1: " + z1_sh.ToString());

                // (7) Open the shares and write the results into the same variables for all parties
                rss.Open(chls, z0_sh, open_z0);
                rss.Open(chls, z1_sh, open_z1);
            };
        };

        // Create tasks for parties 0, 1, and 2
        auto task0 = MakeTask(0);
        auto task1 = MakeTask(1);
        auto task2 = MakeTask(2);

        // Configure network based on party ID (CLI/env) and wait for completion
        net_mgr.AutoConfigure(-1, task0, task1, task2);
        net_mgr.WaitForCompletion();

        // At this point, all parties have the same open_z0 and open_z1
        Logger::DebugLog(LOC, "open_z0: " + ToString(open_z0));
        Logger::DebugLog(LOC, "open_z1: " + ToString(open_z1));

        // Validate the opened values
        if (open_z0 != 5 || open_z1 != 4)
            throw osuCrypto::UnitTestFail("Binary protocol failed: open_z0/open_z1 mismatch");
    }

    Logger::DebugLog(LOC, "Binary3P_EvaluateSelect_Online_Test - Passed");
}

}    // namespace test_fsswm
