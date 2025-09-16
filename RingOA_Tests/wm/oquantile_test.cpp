#include "oquantile_test.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"
#include "RingOA/wm/oquantile.h"
#include "RingOA/wm/plain_wm.h"

namespace {

const std::string kCurrentPath       = ringoa::GetCurrentDirectory();
const std::string kTestOQuantilePath = kCurrentPath + "/data/test/wm/";
const uint64_t    kFixedSeed         = 6;

std::vector<uint64_t> GenerateRandomVector(size_t length, uint64_t sigma) {
    if (length == 0)
        return {};
    static thread_local std::mt19937_64                       rng(kFixedSeed);
    static thread_local std::uniform_int_distribution<size_t> dist(0, 1U << sigma);
    std::vector<uint64_t>                                     result(length);
    for (size_t i = 0; i < length; ++i) {
        result[i] = dist(rng);
    }
    return result;
}

}    // namespace

namespace test_ringoa {

using ringoa::Channels;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::ThreePartyNetworkManager;
using ringoa::ToString, ringoa::ToStringMatrix;
using ringoa::proto::KeyIo;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareVec64;
using ringoa::sharing::RepShareMat64, ringoa::sharing::RepShareView64;
using ringoa::sharing::ShareIo;
using ringoa::wm::OQuantileEvaluator;
using ringoa::wm::OQuantileKey;
using ringoa::wm::OQuantileKeyGenerator;
using ringoa::wm::OQuantileParameters;
using ringoa::wm::WaveletMatrix;

void OQuantile_Offline_Test() {
    Logger::DebugLog(LOC, "OQuantile_Offline_Test...");
    std::vector<OQuantileParameters> params_list = {
        OQuantileParameters(10),
        // OQuantileParameters(15),
        // OQuantileParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t              d  = params.GetDatabaseBitSize();
        uint64_t              s  = params.GetShareSize();
        uint64_t              ds = params.GetDatabaseSize();
        AdditiveSharing2P     ass(s);
        ReplicatedSharing3P   rss(s);
        OQuantileKeyGenerator gen(params, ass, rss);
        FileIo                file_io;
        ShareIo               sh_io;
        KeyIo                 key_io;

        // Generate the database and index
        std::vector<uint64_t> database = GenerateRandomVector(ds - 1, params.GetSigma());
        std::vector<uint64_t> q_arg    = {/* left = */ 123, /* right = */ 456, /* k = */ 100};
        WaveletMatrix         wm(database, params.GetSigma());
        Logger::DebugLog(LOC, "Database: " + ToString(database));
        Logger::DebugLog(LOC, "Left: " + ToString(q_arg[0]) + ", Right: " + ToString(q_arg[1]) + ", k: " + ToString(q_arg[2]));

        // Generate keys
        std::array<OQuantileKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOQuantilePath + "oquantilekey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate replicated shares for the database, query, and position
        std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(wm);
        std::array<RepShareVec64, 3> q_arg_sh = rss.ShareLocal(q_arg);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " (left, right, k) share: " + q_arg_sh[p].ToString());
        }

        // Save data
        std::string db_path    = kTestOQuantilePath + "db_d" + ToString(d);
        std::string q_arg_path = kTestOQuantilePath + "query_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        file_io.WriteBinary(q_arg_path, q_arg);

        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(q_arg_path + "_" + ToString(p), q_arg_sh[p]);
        }

        // Offline setup
        gen.OfflineSetUp(kTestOQuantilePath);
        rss.OfflineSetUp(kTestOQuantilePath + "prf");
    }
    Logger::DebugLog(LOC, "OQuantile_Offline_Test - Passed");
}

void OQuantile_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OQuantile_Online_Test...");
    std::vector<OQuantileParameters> params_list = {
        OQuantileParameters(10),
        // OQuantileParameters(15),
        // OQuantileParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t s  = params.GetShareSize();
        uint64_t nu = params.GetOaParameters().GetParameters().GetTerminateBitsize();

        FileIo file_io;

        uint64_t    result{0};
        std::string key_path   = kTestOQuantilePath + "oquantilekey_d" + ToString(d);
        std::string db_path    = kTestOQuantilePath + "db_d" + ToString(d);
        std::string q_arg_path = kTestOQuantilePath + "query_d" + ToString(d);

        std::vector<uint64_t> database;
        std::vector<uint64_t> q_arg;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(q_arg_path, q_arg);

        // Create a task factory that captures everything needed by value,
        // and captures `result` and `sh_io` by reference.
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                // Set up replicated sharing and evaluator for this party
                ReplicatedSharing3P rss(s);
                AdditiveSharing2P   ass_prev(s), ass_next(s);
                OQuantileEvaluator  eval(params, rss, ass_prev, ass_next);
                Channels            chls(party_id, chl_prev, chl_next);

                // Load this party's key
                OQuantileKey key(party_id, params);
                KeyIo        key_io_local;
                key_io_local.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load this party's shares of the database and query
                RepShareMat64 db_sh;
                RepShareVec64 q_arg_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(q_arg_path + "_" + ToString(party_id), q_arg_sh);

                // Perform the PRF setup step
                eval.OnlineSetUp(party_id, kTestOQuantilePath);
                rss.OnlineSetUp(party_id, kTestOQuantilePath + "prf");

                // Evaluate the rank operation
                RepShare64                 result_sh;
                RepShare64                 left_sh = q_arg_sh.At(0), right_sh = q_arg_sh.At(1), k_sh = q_arg_sh.At(2);
                std::vector<ringoa::block> uv_prev(1U << nu), uv_next(1U << nu);
                eval.EvaluateQuantile_Parallel(chls, key, uv_prev, uv_next,
                                               db_sh, left_sh, right_sh, k_sh, result_sh);

                // Open the resulting share to recover the final value
                rss.Open(chls, result_sh, result);
            };
        };

        // Instantiate tasks for parties 0, 1, and 2
        auto task_p0 = MakeTask(0);
        auto task_p1 = MakeTask(1);
        auto task_p2 = MakeTask(2);

        ThreePartyNetworkManager net_mgr;
        int                      party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();

        Logger::DebugLog(LOC, "Result: " + ToString(result));

        // Verify against the plain-wavelet-matrix rank computation
        WaveletMatrix wm(database, params.GetSigma());
        uint64_t      expected_result = wm.Quantile(q_arg[0], q_arg[1], q_arg[2]);
        if (result != expected_result) {
            throw osuCrypto::UnitTestFail(
                "OWM_Online_Test failed: result = " + ToString(result) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "OQuantile_Online_Test - Passed");
}

}    // namespace test_ringoa
