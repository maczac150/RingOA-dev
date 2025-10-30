#include "obliv_select_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/protocol/obliv_select.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/binary_2p.h"
#include "RingOA/sharing/binary_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace {

const std::string kCurrentPath = ringoa::GetCurrentDirectory();
const std::string kTestOSPath  = kCurrentPath + "/data/test/protocol/";

}    // namespace

namespace test_ringoa {

using ringoa::Channels;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::Mod2N;
using ringoa::ThreePartyNetworkManager;
using ringoa::ToString, ringoa::Format;
using ringoa::fss::EvalType;
using ringoa::proto::KeyIo;
using ringoa::proto::OblivSelectEvaluator;
using ringoa::proto::OblivSelectKey;
using ringoa::proto::OblivSelectKeyGenerator;
using ringoa::proto::OblivSelectParameters;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::BinaryReplicatedSharing3P;
using ringoa::sharing::BinarySharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareBlock;
using ringoa::sharing::RepShareVec64, ringoa::sharing::RepShareVecBlock;
using ringoa::sharing::RepShareView64, ringoa::sharing::RepShareViewBlock;
using ringoa::sharing::ShareIo;

void OblivSelect_Offline_Test() {
    Logger::DebugLog(LOC, "OblivSelect_Offline_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, ringoa::fss::OutputType::kSingleBitMask),
        OblivSelectParameters(10, ringoa::fss::OutputType::kShiftedAdditive),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();

        if (params.GetParameters().GetOutputType() == ringoa::fss::OutputType::kSingleBitMask) {
            // Save keys
            std::string key_path = kTestOSPath + "oskeySBM_d" + ToString(d);
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);

            // Generate the database and index
            std::vector<ringoa::block> database(1U << d);
            for (size_t i = 0; i < database.size(); ++i) {
                database[i] = ringoa::MakeBlock(0, i);
            }
            Logger::DebugLog(LOC, "Database: " + Format(database));

            std::array<RepShareVecBlock, 3> database_sh = brss.ShareLocal(database);
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                Logger::DebugLog(LOC, "Party " + ToString(p) + " shares: " + database_sh[p].ToString());
            }

            // Save data
            std::string db_path = kTestOSPath + "dbSBM_d" + ToString(d);
            file_io.WriteBinary(db_path, database);
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
            }
        } else {
            // Save keys
            std::string key_path = kTestOSPath + "oskeySA_d" + ToString(d);
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);

            // Generate the database and index
            std::vector<uint64_t> database(1U << d);
            for (size_t i = 0; i < database.size(); ++i) {
                database[i] = i;
            }
            Logger::DebugLog(LOC, "Database: " + ToString(database));

            std::array<RepShareVec64, 3> database_sh = brss.ShareLocal(database);
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                Logger::DebugLog(LOC, "Party " + ToString(p) + " db: " + database_sh[p].ToString());
            }

            // Save data
            std::string db_path = kTestOSPath + "dbSA_d" + ToString(d);

            file_io.WriteBinary(db_path, database);
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
            }
        }

        // Generate a random index
        uint64_t index = bss.GenerateRandomValue();
        Logger::DebugLog(LOC, "Index: " + ToString(index));
        std::array<RepShare64, 3> index_sh = brss.ShareLocal(index);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " index share: " + index_sh[p].ToString());
        }
        std::string idx_path = kTestOSPath + "idx_d" + ToString(d);
        file_io.WriteBinary(idx_path, index);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }

        // Offline setup
        brss.OfflineSetUp(kTestOSPath + "prf");
    }
    Logger::DebugLog(LOC, "OblivSelect_Offline_Test - Passed");
}

void OblivSelect_SingleBitMask_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OblivSelect_SingleBitMask_Online_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, ringoa::fss::OutputType::kSingleBitMask),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d = params.GetParameters().GetInputBitsize();
        FileIo   file_io;

        // Variable for the opened result (all parties will write into this)
        ringoa::block result = ringoa::MakeBlock(0, 0);

        std::string key_path = kTestOSPath + "oskeySBM_d" + ToString(d);
        std::string db_path  = kTestOSPath + "dbSBM_d" + ToString(d);
        std::string idx_path = kTestOSPath + "idx_d" + ToString(d);

        std::vector<ringoa::block> database;
        uint64_t                   index;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(idx_path, index);

        // Helper that returns a task lambda for a given party_id
        auto MakeTask = [&](int party_id) {
            // Capture d, params, paths, and references to opened-result variable
            return [=, &result](osuCrypto::Channel &chl_next,
                                osuCrypto::Channel &chl_prev) {
                // (1) Set up the binary replicated-sharing object and OblivSelectEvaluator
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(party_id, chl_prev, chl_next);

                // (2) Load the party’s OblivSelectKey
                OblivSelectKey key(party_id, params);
                KeyIo          key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (3) Load the party’s database share and index share
                RepShareVecBlock database_sh;
                RepShare64       index_sh;
                ShareIo          sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                // (4) Setup PRF keys
                brss.OnlineSetUp(party_id, kTestOSPath + "prf");

                // (5) Evaluate OblivSelect on binary data
                RepShareBlock result_sh;
                eval.Evaluate(
                    chls,
                    key,
                    RepShareViewBlock(database_sh),
                    index_sh,
                    result_sh);

                // (6) Open the result and write into the common result variable
                brss.Open(chls, result_sh, result);
            };
        };

        // Create tasks for parties 0, 1, and 2
        auto task0 = MakeTask(0);
        auto task1 = MakeTask(1);
        auto task2 = MakeTask(2);

        // Configure network based on party ID and wait for completion
        ThreePartyNetworkManager net_mgr;
        int                      party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task0, task1, task2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + Format(result));

        if (result != database[index]) {
            throw osuCrypto::UnitTestFail(
                "OblivSelect_SingleBitMask_Online_Test failed: result = " + Format(result) +
                ", expected = " + Format(database[index]));
        }
    }

    Logger::DebugLog(LOC, "OblivSelect_SingleBitMask_Online_Test - Passed");
}

void OblivSelect_ShiftedAdditive_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OblivSelect_ShiftedAdditive_Online_Test...");
    std::vector<OblivSelectParameters> params_list = {
        OblivSelectParameters(10, ringoa::fss::OutputType::kShiftedAdditive),
        // OblivSelectParameters(15),
        // OblivSelectParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetParameters().GetInputBitsize();
        uint64_t nu = params.GetParameters().GetTerminateBitsize();
        FileIo   file_io;
        ShareIo  sh_io;

        uint64_t              result{0};
        std::string           key_path = kTestOSPath + "oskeySA_d" + ToString(d);
        std::string           db_path  = kTestOSPath + "dbSA_d" + ToString(d);
        std::string           idx_path = kTestOSPath + "idx_d" + ToString(d);
        std::vector<uint64_t> database;
        uint64_t              index;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(idx_path, index);

        // Define the task for each party
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(party_id, chl_prev, chl_next);

                // Load keys
                OblivSelectKey key(party_id, params);
                KeyIo          key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load data
                RepShareVec64 database_sh;
                RepShare64    index_sh;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                std::vector<ringoa::block> uv_prev(1U << nu), uv_next(1U << nu);

                // Setup the PRF keys
                brss.OnlineSetUp(party_id, kTestOSPath + "prf");

                // Evaluate
                RepShare64 result_sh;
                eval.Evaluate(chls, key, uv_prev, uv_next, RepShareView64(database_sh), index_sh, result_sh);

                // Open the result
                uint64_t local_res1 = 0;
                brss.Open(chls, result_sh, local_res1);
                result = local_res1;
            };
        };

        // Create tasks for each party
        auto task_p0 = MakeTask(0);
        auto task_p1 = MakeTask(1);
        auto task_p2 = MakeTask(2);

        ThreePartyNetworkManager net_mgr;
        // Configure network based on party ID and wait for completion
        int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + ToString(result));

        if (result != database[index])
            throw osuCrypto::UnitTestFail("OblivSelect_ShiftedAdditive_Online_Test failed: result = " + ToString(result) +
                                          ", expected = " + ToString(database[index]));
    }
    Logger::DebugLog(LOC, "OblivSelect_ShiftedAdditive_Online_Test - Passed");
}

}    // namespace test_ringoa
