#include "owm_test.h"

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
#include "RingOA/wm/owm.h"
#include "RingOA/wm/plain_wm.h"

namespace {

const std::string kCurrentPath = ringoa::GetCurrentDirectory();
const std::string kTestOWMPath = kCurrentPath + "/data/test/wm/";
const uint64_t    kFixedSeed   = 6;

std::string GenerateRandomString(size_t length, const std::string &charset = "ATGC") {
    if (charset.empty() || length == 0)
        return "";
    static thread_local std::mt19937_64                       rng(kFixedSeed);
    static thread_local std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string                                               result(length, '\0');
    for (size_t i = 0; i < length; ++i) {
        result[i] = charset[dist(rng)];
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
using ringoa::sharing::RepShareBlock, ringoa::sharing::RepShareVecBlock;
using ringoa::sharing::RepShareMat64, ringoa::sharing::RepShareView64;
using ringoa::sharing::RepShareMatBlock, ringoa::sharing::RepShareViewBlock;
using ringoa::sharing::ShareIo;
using ringoa::wm::FMIndex;
using ringoa::wm::OWMEvaluator;
using ringoa::wm::OWMKey;
using ringoa::wm::OWMKeyGenerator;
using ringoa::wm::OWMParameters;

void OWM_Offline_Test() {
    Logger::DebugLog(LOC, "OWM_Offline_Test...");
    std::vector<OWMParameters> params_list = {
        OWMParameters(10),
        // OWMParameters(10),
        // OWMParameters(15),
        // OWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t            d  = params.GetDatabaseBitSize();
        uint64_t            ds = params.GetDatabaseSize();
        AdditiveSharing2P   ass(d);
        ReplicatedSharing3P rss(d);
        OWMKeyGenerator     gen(params, ass, rss);
        FileIo              file_io;
        ShareIo             sh_io;
        KeyIo               key_io;

        // Generate the database and index
        std::string           database = GenerateRandomString(ds - 2);
        FMIndex               fm(database);
        std::vector<uint64_t> query    = {0, 1, 0};
        uint64_t              position = rss.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + ToString(query));
        Logger::DebugLog(LOC, "Position: " + ToString(position));

        // Generate keys
        std::array<OWMKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOWMPath + "owmkey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate replicated shares for the database, query, and position
        std::array<RepShareMat64, 3> db_sh       = gen.GenerateDatabaseU64Share(fm);
        std::array<RepShareVec64, 3> query_sh    = rss.ShareLocal(query);
        std::array<RepShare64, 3>    position_sh = rss.ShareLocal(position);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToString());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " position share: " + position_sh[p].ToString());
        }

        // Save data
        std::string db_path       = kTestOWMPath + "db_d" + ToString(d);
        std::string query_path    = kTestOWMPath + "query_d" + ToString(d);
        std::string position_path = kTestOWMPath + "position_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        file_io.WriteBinary(query_path, query);
        file_io.WriteBinary(position_path, position);

        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
            sh_io.SaveShare(position_path + "_" + ToString(p), position_sh[p]);
        }

        // Offline setup
        gen.GetRingOaKeyGenerator().OfflineSetUp(params.GetSigma(), kTestOWMPath);
        rss.OfflineSetUp(kTestOWMPath + "prf");
    }
    Logger::DebugLog(LOC, "OWM_Offline_Test - Passed");
}

void OWM_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OWM_Online_Test...");
    std::vector<OWMParameters> params_list = {
        OWMParameters(10, 3),
        // OWMParameters(15),
        // OWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t nu = params.GetOaParameters().GetParameters().GetTerminateBitsize();

        FileIo file_io;

        uint64_t    result{0};
        std::string key_path      = kTestOWMPath + "owmkey_d" + ToString(d);
        std::string db_path       = kTestOWMPath + "db_d" + ToString(d);
        std::string query_path    = kTestOWMPath + "query_d" + ToString(d);
        std::string position_path = kTestOWMPath + "position_d" + ToString(d);

        std::string           database;
        std::vector<uint64_t> query;
        uint64_t              position;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(query_path, query);
        file_io.ReadBinary(position_path, position);

        // Create a task factory that captures everything needed by value,
        // and captures `result` and `sh_io` by reference.
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                // Set up replicated sharing and evaluator for this party
                ReplicatedSharing3P rss(d);
                AdditiveSharing2P   ass_prev(d), ass_next(d);
                OWMEvaluator        eval(params, rss, ass_prev, ass_next);
                Channels            chls(party_id, chl_prev, chl_next);

                // Load this party's key
                OWMKey key(party_id, params);
                KeyIo  key_io_local;
                key_io_local.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load this party's shares of the database, query, and position
                RepShareMat64 db_sh;
                RepShareVec64 query_sh;
                RepShare64    position_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(party_id), query_sh);
                sh_io.LoadShare(position_path + "_" + ToString(party_id), position_sh);

                // Perform the PRF setup step
                eval.GetRingOaEvaluator().OnlineSetUp(party_id, kTestOWMPath);
                rss.OnlineSetUp(party_id, kTestOWMPath + "prf");

                // Evaluate the rank operation
                RepShare64                 result_sh;
                std::vector<ringoa::block> uv_prev(1U << nu), uv_next(1U << nu);
                eval.EvaluateRankCF(chls, key, uv_prev, uv_next, db_sh, RepShareView64(query_sh), position_sh, result_sh);

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
        FMIndex  fmi(database);
        uint64_t expected_result = fmi.GetWaveletMatrix().RankCF(2, position);
        if (result != expected_result) {
            throw osuCrypto::UnitTestFail(
                "OWM_Online_Test failed: result = " + ToString(result) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "OWM_Online_Test - Passed");
}

}    // namespace test_ringoa
