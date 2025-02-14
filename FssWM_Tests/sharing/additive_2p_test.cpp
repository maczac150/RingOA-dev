#include "additive_2p_test.h"

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath      = fsswm::GetCurrentDirectory();
const std::string kTestAdditivePath = kCurrentPath + "/data/test/ss/";

}    // namespace

namespace test_fsswm {

using fsswm::sharing::AdditiveSharing2P;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ToString;
using fsswm::TwoPartyNetworkManager;

const std::vector<uint32_t> kBitsizes = {
    5,
    10,
    15,
    20,
};

void Additive2P_EvaluateAdd_Offline_Test() {
    Logger::DebugLog(LOC, "Additive2P_EvaluateAdd_Offline_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        AdditiveSharing2P ss(bitsize);
        FileIo            file_io;

        // Generate input
        uint32_t                x     = 5;
        uint32_t                y     = 4;
        std::array<uint32_t, 2> x_arr = {1, 2};
        std::array<uint32_t, 2> y_arr = {5, 4};
        std::vector<uint32_t>   x_vec = {1, 2, 3, 4, 5};
        std::vector<uint32_t>   y_vec = {5, 4, 3, 2, 1};

        // Generate shares
        std::pair<uint32_t, uint32_t>                               x_sh     = ss.Share(x);
        std::pair<uint32_t, uint32_t>                               y_sh     = ss.Share(y);
        std::pair<std::array<uint32_t, 2>, std::array<uint32_t, 2>> x_arr_sh = ss.Share(x_arr);
        std::pair<std::array<uint32_t, 2>, std::array<uint32_t, 2>> y_arr_sh = ss.Share(y_arr);
        std::pair<std::vector<uint32_t>, std::vector<uint32_t>>     x_vec_sh = ss.Share(x_vec);
        std::pair<std::vector<uint32_t>, std::vector<uint32_t>>     y_vec_sh = ss.Share(y_vec);

        Logger::DebugLog(LOC, "x: " + std::to_string(x) + ", y: " + std::to_string(y));
        Logger::DebugLog(LOC, "x_0: " + std::to_string(x_sh.first) + ", x_1: " + std::to_string(x_sh.second));
        Logger::DebugLog(LOC, "y_0: " + std::to_string(y_sh.first) + ", y_1: " + std::to_string(y_sh.second));
        Logger::DebugLog(LOC, "x_arr: " + ToString(x_arr) + ", y_arr: " + ToString(y_arr));
        Logger::DebugLog(LOC, "x_arr_0: " + ToString(x_arr_sh.first) + ", x_arr_1: " + ToString(x_arr_sh.second));
        Logger::DebugLog(LOC, "y_arr_0: " + ToString(y_arr_sh.first) + ", y_arr_1: " + ToString(y_arr_sh.second));
        Logger::DebugLog(LOC, "x_vec: " + ToString(x_vec) + ", y_vec: " + ToString(y_vec));
        Logger::DebugLog(LOC, "x_vec_0: " + ToString(x_vec_sh.first) + ", x_vec_1: " + ToString(x_vec_sh.second));
        Logger::DebugLog(LOC, "y_vec_0: " + ToString(y_vec_sh.first) + ", y_vec_1: " + ToString(y_vec_sh.second));

        // Save input
        std::string x_path = kTestAdditivePath + "x_n" + std::to_string(bitsize);
        std::string y_path = kTestAdditivePath + "y_n" + std::to_string(bitsize);
        file_io.WriteToFile(x_path + "_0", x_sh.first);
        file_io.WriteToFile(x_path + "_1", x_sh.second);
        file_io.WriteToFile(y_path + "_0", y_sh.first);
        file_io.WriteToFile(y_path + "_1", y_sh.second);
        file_io.WriteToFile(x_path + "_arr_0", x_arr_sh.first);
        file_io.WriteToFile(x_path + "_arr_1", x_arr_sh.second);
        file_io.WriteToFile(y_path + "_arr_0", y_arr_sh.first);
        file_io.WriteToFile(y_path + "_arr_1", y_arr_sh.second);
        file_io.WriteToFile(x_path + "_vec_0", x_vec_sh.first);
        file_io.WriteToFile(x_path + "_vec_1", x_vec_sh.second);
        file_io.WriteToFile(y_path + "_vec_0", y_vec_sh.first);
        file_io.WriteToFile(y_path + "_vec_1", y_vec_sh.second);

        // Load input
        uint32_t                x_0, x_1, y_0, y_1;
        std::array<uint32_t, 2> x_arr_0, x_arr_1, y_arr_0, y_arr_1;
        std::vector<uint32_t>   x_vec_0, x_vec_1, y_vec_0, y_vec_1;
        file_io.ReadFromFile(x_path + "_0", x_0);
        file_io.ReadFromFile(x_path + "_1", x_1);
        file_io.ReadFromFile(y_path + "_0", y_0);
        file_io.ReadFromFile(y_path + "_1", y_1);
        file_io.ReadFromFile(x_path + "_arr_0", x_arr_0);
        file_io.ReadFromFile(x_path + "_arr_1", x_arr_1);
        file_io.ReadFromFile(y_path + "_arr_0", y_arr_0);
        file_io.ReadFromFile(y_path + "_arr_1", y_arr_1);
        file_io.ReadFromFile(x_path + "_vec_0", x_vec_0);
        file_io.ReadFromFile(x_path + "_vec_1", x_vec_1);
        file_io.ReadFromFile(y_path + "_vec_0", y_vec_0);
        file_io.ReadFromFile(y_path + "_vec_1", y_vec_1);

        // Reconstruct
        uint32_t                x_rec, y_rec;
        std::array<uint32_t, 2> x_arr_rec, y_arr_rec;
        std::vector<uint32_t>   x_vec_rec, y_vec_rec;
        ss.ReconstLocal(x_0, x_1, x_rec);
        ss.ReconstLocal(y_0, y_1, y_rec);
        ss.ReconstLocal(x_arr_0, x_arr_1, x_arr_rec);
        ss.ReconstLocal(y_arr_0, y_arr_1, y_arr_rec);
        ss.ReconstLocal(x_vec_0, x_vec_1, x_vec_rec);
        ss.ReconstLocal(y_vec_0, y_vec_1, y_vec_rec);

        Logger::DebugLog(LOC, "x_rec: " + std::to_string(x_rec) + ", y_rec: " + std::to_string(y_rec));
        Logger::DebugLog(LOC, "x_arr_rec: " + ToString(x_arr_rec) + ", y_arr_rec: " + ToString(y_arr_rec));
        Logger::DebugLog(LOC, "x_vec_rec: " + ToString(x_vec_rec) + ", y_vec_rec: " + ToString(y_vec_rec));
    }

    Logger::DebugLog(LOC, "Additive2P_EvaluateAdd_Offline_Test - Passed");
}

void Additive2P_EvaluateAdd_Online_Test() {
    Logger::DebugLog(LOC, "Additive2P_EvaluateAdd_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        AdditiveSharing2P ss(bitsize);
        FileIo            file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("Additive2P_EvaluateAdd_Online_Test");

        uint32_t                z_0, z_1, z;
        std::array<uint32_t, 2> z_arr_0, z_arr_1, z_arr;
        std::vector<uint32_t>   z_vec_0, z_vec_1, z_vec;
        std::string             x_path = kTestAdditivePath + "x_n" + std::to_string(bitsize);
        std::string             y_path = kTestAdditivePath + "y_n" + std::to_string(bitsize);

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            uint32_t                x_0, y_0;
            std::array<uint32_t, 2> x_arr_0, y_arr_0;
            std::vector<uint32_t>   x_vec_0, y_vec_0;

            // Load input
            file_io.ReadFromFile(x_path + "_0", x_0);
            file_io.ReadFromFile(y_path + "_0", y_0);
            file_io.ReadFromFile(x_path + "_arr_0", x_arr_0);
            file_io.ReadFromFile(y_path + "_arr_0", y_arr_0);
            file_io.ReadFromFile(x_path + "_vec_0", x_vec_0);
            file_io.ReadFromFile(y_path + "_vec_0", y_vec_0);

            // Evaluate Add
            ss.EvaluateAdd(x_0, y_0, z_0);
            ss.EvaluateAdd(x_arr_0, y_arr_0, z_arr_0);
            ss.EvaluateAdd(x_vec_0, y_vec_0, z_vec_0);

            Logger::DebugLog(LOC, "z_0: " + std::to_string(z_0));
            Logger::DebugLog(LOC, "z_arr_0: " + ToString(z_arr_0));
            Logger::DebugLog(LOC, "z_vec_0: " + ToString(z_vec_0));

            // Reconstruct
            ss.Reconst(0, chl, z_0, z_1, z);
            ss.Reconst(0, chl, z_arr_0, z_arr_1, z_arr);
            ss.Reconst(0, chl, z_vec_0, z_vec_1, z_vec);
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            uint32_t                x_1, y_1;
            std::array<uint32_t, 2> x_arr_1, y_arr_1;
            std::vector<uint32_t>   x_vec_1, y_vec_1;

            // Load input
            file_io.ReadFromFile(x_path + "_1", x_1);
            file_io.ReadFromFile(y_path + "_1", y_1);
            file_io.ReadFromFile(x_path + "_arr_1", x_arr_1);
            file_io.ReadFromFile(y_path + "_arr_1", y_arr_1);
            file_io.ReadFromFile(x_path + "_vec_1", x_vec_1);
            file_io.ReadFromFile(y_path + "_vec_1", y_vec_1);

            // Evaluate Add
            ss.EvaluateAdd(x_1, y_1, z_1);
            ss.EvaluateAdd(x_arr_1, y_arr_1, z_arr_1);
            ss.EvaluateAdd(x_vec_1, y_vec_1, z_vec_1);

            Logger::DebugLog(LOC, "z_1: " + std::to_string(z_1));
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

        Logger::DebugLog(LOC, "z: " + std::to_string(z));
        Logger::DebugLog(LOC, "z_arr: " + ToString(z_arr));
        Logger::DebugLog(LOC, "z_vec: " + ToString(z_vec));

        // Validate the result
        if (z != 9 || z_arr != std::array<uint32_t, 2>{6, 6} || z_vec != std::vector<uint32_t>{6, 6, 6, 6, 6})
            throw oc::UnitTestFail("EvaluateAdd failed.");

        Logger::DebugLog(LOC, "Additive2P_EvaluateAdd_Online_Test - Passed");
    }
}

void Additive2P_EvaluateMult_Offline_Test() {
    Logger::DebugLog(LOC, "Additive2P_EvaluateMult_Offline_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        AdditiveSharing2P ss(bitsize);

        // Generate beaver triples
        std::string triple_path = kTestAdditivePath + "triple_n" + std::to_string(bitsize);
        ss.OfflineSetUp(3, triple_path);

        ss.OnlineSetUp(0, triple_path);
        ss.PrintTriples();

        ss.OnlineSetUp(1, triple_path);
        ss.PrintTriples();
    }
    Logger::DebugLog(LOC, "Additive2P_EvaluateMult_Offline_Test - Passed");
}

void Additive2P_EvaluateMult_Online_Test() {
    Logger::DebugLog(LOC, "Additive2P_EvaluateMult_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        FileIo file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("Additive2P_EvaluateMult_Test");

        uint32_t                z_0, z_1, z;
        std::array<uint32_t, 2> z_arr_0, z_arr_1, z_arr;
        std::string             x_path      = kTestAdditivePath + "x_n" + std::to_string(bitsize);
        std::string             y_path      = kTestAdditivePath + "y_n" + std::to_string(bitsize);
        std::string             triple_path = kTestAdditivePath + "triple_n" + std::to_string(bitsize);

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            AdditiveSharing2P       ss(bitsize);
            uint32_t                x_0, y_0;
            std::array<uint32_t, 2> x_arr_0, y_arr_0;

            // Load input
            file_io.ReadFromFile(x_path + "_0", x_0);
            file_io.ReadFromFile(y_path + "_0", y_0);
            file_io.ReadFromFile(x_path + "_arr_0", x_arr_0);
            file_io.ReadFromFile(y_path + "_arr_0", y_arr_0);

            // Setup additive sharing
            ss.OnlineSetUp(0, triple_path);

            Logger::DebugLog(LOC, "Before: Remaining triples: " + std::to_string(ss.GetRemainingTripleCount()));

            // Evaluate Mult
            ss.EvaluateMult(0, chl, x_0, y_0, z_0);
            ss.EvaluateMult(0, chl, x_arr_0, y_arr_0, z_arr_0);

            Logger::DebugLog(LOC, "After: Remaining triples: " + std::to_string(ss.GetRemainingTripleCount()));

            // Reconstruct
            ss.Reconst(0, chl, z_0, z_1, z);
            ss.Reconst(0, chl, z_arr_0, z_arr_1, z_arr);

            Logger::DebugLog(LOC, "z_0: " + std::to_string(z_0));
            Logger::DebugLog(LOC, "z_arr_0: " + ToString(z_arr_0));
        };

        // Client task

        auto client_task = [&](oc::Channel &chl) {
            AdditiveSharing2P       ss(bitsize);
            uint32_t                x_1, y_1;
            std::array<uint32_t, 2> x_arr_1, y_arr_1;

            // Load input
            file_io.ReadFromFile(x_path + "_1", x_1);
            file_io.ReadFromFile(y_path + "_1", y_1);
            file_io.ReadFromFile(x_path + "_arr_1", x_arr_1);
            file_io.ReadFromFile(y_path + "_arr_1", y_arr_1);

            // Setup additive sharing
            ss.OnlineSetUp(1, triple_path);

            // Evaluate Mult
            ss.EvaluateMult(1, chl, x_1, y_1, z_1);
            ss.EvaluateMult(1, chl, x_arr_1, y_arr_1, z_arr_1);

            // Reconstruct
            ss.Reconst(1, chl, z_0, z_1, z);
            ss.Reconst(1, chl, z_arr_0, z_arr_1, z_arr);

            Logger::DebugLog(LOC, "z_1: " + std::to_string(z_1));
            Logger::DebugLog(LOC, "z_arr_1: " + ToString(z_arr_1));
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, server_task, client_task);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "z: " + std::to_string(z));
        Logger::DebugLog(LOC, "z_arr: " + ToString(z_arr));

        // Validate the result
        if (z != 20 || z_arr != std::array<uint32_t, 2>{5, 8})
            throw oc::UnitTestFail("EvaluateMult failed.");
    }
    Logger::DebugLog(LOC, "Additive2P_EvaluateMult_Online_Test - Passed");
}

void Additive2P_EvaluateSelect_Offline_Test() {
    Logger::DebugLog(LOC, "Additive2P_EvaluateSelect_Offline_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        AdditiveSharing2P ss(bitsize);
        FileIo            file_io;

        // Generate conditional shares
        uint32_t                c     = 1;
        std::array<uint32_t, 2> c_arr = {1, 0};

        // Generate conditional shares
        std::pair<uint32_t, uint32_t>                               c_sh     = ss.Share(c);
        std::pair<std::array<uint32_t, 2>, std::array<uint32_t, 2>> c_arr_sh = ss.Share(c_arr);

        Logger::DebugLog(LOC, "c: " + std::to_string(c));
        Logger::DebugLog(LOC, "c_0: " + std::to_string(c_sh.first) + ", c_1: " + std::to_string(c_sh.second));
        Logger::DebugLog(LOC, "c_arr: " + ToString(c_arr));
        Logger::DebugLog(LOC, "c_arr_0: " + ToString(c_arr_sh.first) + ", c_arr_1: " + ToString(c_arr_sh.second));

        // Save conditional shares
        std::string c_path = kTestAdditivePath + "c_n" + std::to_string(bitsize);
        file_io.WriteToFile(c_path + "_0", c_sh.first);
        file_io.WriteToFile(c_path + "_1", c_sh.second);
        file_io.WriteToFile(c_path + "_arr_0", c_arr_sh.first);
        file_io.WriteToFile(c_path + "_arr_1", c_arr_sh.second);
    }
    Logger::DebugLog(LOC, "Additive2P_EvaluateSelect_Offline_Test - Passed");
}

void Additive2P_EvaluateSelect_Online_Test() {
    Logger::DebugLog(LOC, "Additive2P_EvaluateSelect_Online_Test...");

    for (const uint32_t bitsize : kBitsizes) {
        FileIo file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("Additive2P_EvaluateSelect_Test");

        uint32_t                z_0, z_1, z;
        std::array<uint32_t, 2> z_arr_0, z_arr_1, z_arr;
        std::string             x_path      = kTestAdditivePath + "x_n" + std::to_string(bitsize);
        std::string             y_path      = kTestAdditivePath + "y_n" + std::to_string(bitsize);
        std::string             c_path      = kTestAdditivePath + "c_n" + std::to_string(bitsize);
        std::string             triple_path = kTestAdditivePath + "triple_n" + std::to_string(bitsize);

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            AdditiveSharing2P       ss(bitsize);
            uint32_t                x_0, y_0, c_0;
            std::array<uint32_t, 2> x_arr_0, y_arr_0, c_arr_0;

            // Load input
            file_io.ReadFromFile(x_path + "_0", x_0);
            file_io.ReadFromFile(y_path + "_0", y_0);
            file_io.ReadFromFile(c_path + "_0", c_0);
            file_io.ReadFromFile(x_path + "_arr_0", x_arr_0);
            file_io.ReadFromFile(y_path + "_arr_0", y_arr_0);
            file_io.ReadFromFile(c_path + "_arr_0", c_arr_0);

            // Setup additive sharing
            ss.OnlineSetUp(0, triple_path);

            // Evaluate Select
            ss.EvaluateSelect(0, chl, x_0, y_0, c_0, z_0);
            ss.EvaluateSelect(0, chl, x_arr_0, y_arr_0, c_arr_0, z_arr_0);

            // Reconstruct
            ss.Reconst(0, chl, z_0, z_1, z);
            ss.Reconst(0, chl, z_arr_0, z_arr_1, z_arr);

            Logger::DebugLog(LOC, "[P0] z_0: " + std::to_string(z_0));
            Logger::DebugLog(LOC, "[P0] z_arr_0: " + ToString(z_arr_0));
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            AdditiveSharing2P       ss(bitsize);
            uint32_t                x_1, y_1, c_1;
            std::array<uint32_t, 2> x_arr_1, y_arr_1, c_arr_1;

            // Load input
            file_io.ReadFromFile(x_path + "_1", x_1);
            file_io.ReadFromFile(y_path + "_1", y_1);
            file_io.ReadFromFile(c_path + "_1", c_1);
            file_io.ReadFromFile(x_path + "_arr_1", x_arr_1);
            file_io.ReadFromFile(y_path + "_arr_1", y_arr_1);
            file_io.ReadFromFile(c_path + "_arr_1", c_arr_1);

            // Setup additive sharing
            ss.OnlineSetUp(1, triple_path);

            // Evaluate Select
            ss.EvaluateSelect(1, chl, x_1, y_1, c_1, z_1);
            ss.EvaluateSelect(1, chl, x_arr_1, y_arr_1, c_arr_1, z_arr_1);

            // Reconstruct
            ss.Reconst(1, chl, z_0, z_1, z);
            ss.Reconst(1, chl, z_arr_0, z_arr_1, z_arr);

            Logger::DebugLog(LOC, "[P1] z_1: " + std::to_string(z_1));
            Logger::DebugLog(LOC, "[P1] z_arr_1: " + ToString(z_arr_1));
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(-1, server_task, client_task);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "z: " + std::to_string(z));
        Logger::DebugLog(LOC, "z_arr: " + ToString(z_arr));

        // Validate the result
        // Select(x, y, c)  = x if c = 0 otherwise y
        // x = [1, 2, 3, 4, 5]
        // y = [5, 4, 3, 2, 1]
        // c = [1, 0, 1, 0, 1]
        // z = [5, 2, 3, 4, 1]
        if (z != 4 || z_arr != std::array<uint32_t, 2>{5, 2})
            throw oc::UnitTestFail("EvaluateSelect failed.");
    }

    Logger::DebugLog(LOC, "Additive2P_EvaluateSelect_Online_Test - Passed");
}

}    // namespace test_fsswm
