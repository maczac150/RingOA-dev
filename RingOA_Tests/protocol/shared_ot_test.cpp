#include "shared_ot_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/protocol/shared_ot.h"
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
using ringoa::proto::SharedOtEvaluator;
using ringoa::proto::SharedOtKey;
using ringoa::proto::SharedOtKeyGenerator;
using ringoa::proto::SharedOtParameters;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::BinaryReplicatedSharing3P;
using ringoa::sharing::BinarySharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareBlock;
using ringoa::sharing::RepShareVec64, ringoa::sharing::RepShareVecBlock;
using ringoa::sharing::RepShareView64, ringoa::sharing::RepShareViewBlock;
using ringoa::sharing::ShareIo;

void SharedOt_Offline_Test() {
    Logger::DebugLog(LOC, "SharedOt_Offline_Test...");
    std::vector<SharedOtParameters> params_list = {
        SharedOtParameters(10),
        SharedOtParameters(11, EvalType::kIterative),
        // SharedOtParameters(15),
        // SharedOtParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t             d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P    ass(d);
        ReplicatedSharing3P  rss(d);
        SharedOtKeyGenerator gen(params, ass);
        FileIo               file_io;
        ShareIo              sh_io;
        KeyIo                key_io;

        // Generate keys
        std::array<SharedOtKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOSPath + "sharedotkey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::vector<uint64_t> database(1U << d);
        for (size_t i = 0; i < database.size(); ++i) {
            database[i] = i;
        }
        Logger::DebugLog(LOC, "Database: " + ToString(database));

        std::array<RepShareVec64, 3> database_sh = rss.ShareLocal(database);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " db: " + database_sh[p].ToString());
        }

        // Save data
        std::string db_path = kTestOSPath + "sharedotdb_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
        }

        // Generate a random index
        uint64_t index = ass.GenerateRandomValue();
        Logger::DebugLog(LOC, "Index: " + ToString(index));
        std::array<RepShare64, 3> index_sh = rss.ShareLocal(index);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " index share: " + index_sh[p].ToString());
        }
        std::string idx_path = kTestOSPath + "sharedotidx_d" + ToString(d);
        file_io.WriteBinary(idx_path, index);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }

        // Offline setup
        rss.OfflineSetUp(kTestOSPath + "prf");
    }
    Logger::DebugLog(LOC, "SharedOt_Offline_Test - Passed");
}

void SharedOt_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "SharedOt_Additive_Online_Test...");
    std::vector<SharedOtParameters> params_list = {
        SharedOtParameters(10),
        SharedOtParameters(11, EvalType::kIterative),
        // SharedOtParameters(15),
        // SharedOtParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d = params.GetParameters().GetInputBitsize();
        FileIo   file_io;
        ShareIo  sh_io;

        uint64_t              result{0};
        std::string           key_path = kTestOSPath + "sharedotkey_d" + ToString(d);
        std::string           db_path  = kTestOSPath + "sharedotdb_d" + ToString(d);
        std::string           idx_path = kTestOSPath + "sharedotidx_d" + ToString(d);
        std::vector<uint64_t> database;
        uint64_t              index;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(idx_path, index);

        // Define the task for each party
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                ReplicatedSharing3P rss(d);
                SharedOtEvaluator   eval(params, rss);
                Channels            chls(party_id, chl_prev, chl_next);

                // Load keys
                SharedOtKey key(party_id, params);
                KeyIo       key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load data
                RepShareVec64 database_sh;
                RepShare64    index_sh;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                std::vector<uint64_t> uv_prev(1U << d), uv_next(1U << d);

                // Setup the PRF keys
                rss.OnlineSetUp(party_id, kTestOSPath + "prf");

                // Evaluate
                RepShare64 result_sh;
                eval.Evaluate(chls, key, uv_prev, uv_next, RepShareView64(database_sh), index_sh, result_sh);

                // Open the result
                uint64_t local_res1 = 0;
                rss.Open(chls, result_sh, local_res1);
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
            throw osuCrypto::UnitTestFail("SharedOt_Online_Test failed: result = " + ToString(result) +
                                          ", expected = " + ToString(database[index]));
    }
    Logger::DebugLog(LOC, "SharedOt_Online_Test - Passed");
}

}    // namespace test_ringoa
