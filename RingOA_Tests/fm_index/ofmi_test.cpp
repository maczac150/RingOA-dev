#include "ofmi_test.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/fm_index/ofmi.h"
#include "RingOA/fm_index/sotfmi.h"
#include "RingOA/protocol/key_io.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/binary_2p.h"
#include "RingOA/sharing/binary_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"
#include "RingOA/wm/plain_wm.h"

namespace {

const std::string kCurrentPath  = ringoa::GetCurrentDirectory();
const std::string kTestOFMIPath = kCurrentPath + "/data/test/fmi/";
const uint64_t    kFixedSeed    = 6;

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
using ringoa::CreateSequence;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::ThreePartyNetworkManager;
using ringoa::ToString;
using ringoa::fm_index::OFMIEvaluator;
using ringoa::fm_index::OFMIKey;
using ringoa::fm_index::OFMIKeyGenerator;
using ringoa::fm_index::OFMIParameters;
using ringoa::fm_index::SotFMIEvaluator;
using ringoa::fm_index::SotFMIKey;
using ringoa::fm_index::SotFMIKeyGenerator;
using ringoa::fm_index::SotFMIParameters;
using ringoa::proto::KeyIo;
using ringoa::sharing::AdditiveSharing2P, ringoa::sharing::BinarySharing2P;
using ringoa::sharing::ReplicatedSharing3P, ringoa::sharing::BinaryReplicatedSharing3P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareBlock;
using ringoa::sharing::RepShareMat64, ringoa::sharing::RepShareMatBlock;
using ringoa::sharing::RepShareVec64, ringoa::sharing::RepShareVecBlock;
using ringoa::sharing::ShareIo;
using ringoa::wm::FMIndex;

void SotFMI_Offline_Test() {
    Logger::DebugLog(LOC, "SotFMI_Offline_Test...");
    std::vector<SotFMIParameters> params_list = {
        SotFMIParameters(10, 10),
        // SotFMIParameters(10),
        // SotFMIParameters(15),
        // SotFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t            d  = params.GetDatabaseBitSize();
        uint64_t            ds = params.GetDatabaseSize();
        uint64_t            qs = params.GetQuerySize();
        AdditiveSharing2P   ass(d);
        ReplicatedSharing3P rss(d);
        SotFMIKeyGenerator  gen(params, ass, rss);
        FileIo              file_io;
        ShareIo             sh_io;
        KeyIo               key_io;

        // Generate keys
        std::array<SotFMIKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOFMIPath + "ofmikey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::string database = GenerateRandomString(ds - 2);
        FMIndex     fm(database);
        std::string query = GenerateRandomString(qs);
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + query);

        std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(fm);
        std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToStringMatrix());
        }

        // Save data
        std::string db_path    = kTestOFMIPath + "db_d" + ToString(d);
        std::string query_path = kTestOFMIPath + "query_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        file_io.WriteBinary(query_path, query);

        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
        }

        // Offline setup
        rss.OfflineSetUp(kTestOFMIPath + "prf");
    }
    Logger::DebugLog(LOC, "SotFMI_Offline_Test - Passed");
}

void SotFMI_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "SotFMI_Online_Test...");
    std::vector<SotFMIParameters> params_list = {
        SotFMIParameters(10, 10),
        // SotFMIParameters(10),
        // SotFMIParameters(15),
        // SotFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t qs = params.GetQuerySize();

        FileIo file_io;

        std::vector<uint64_t> result;
        std::string           key_path   = kTestOFMIPath + "ofmikey_d" + ToString(d);
        std::string           db_path    = kTestOFMIPath + "db_d" + ToString(d);
        std::string           query_path = kTestOFMIPath + "query_d" + ToString(d);

        std::string database;
        std::string query;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(query_path, query);

        // Factory to create a per-party task
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                // Set up replicated sharing and evaluator
                ReplicatedSharing3P rss(d);
                AdditiveSharing2P   ass(d);
                SotFMIEvaluator     eval(params, rss, ass);
                Channels            chls(party_id, chl_prev, chl_next);

                // Load this party's key
                SotFMIKey key(party_id, params);
                KeyIo     key_io_local;
                key_io_local.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load this party's shares of the database and query
                RepShareMat64 db_sh;
                RepShareMat64 query_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(party_id), query_sh);

                // Perform the PRF setup step
                rss.OnlineSetUp(party_id, kTestOFMIPath + "prf");

                // Evaluate the longest-prefix-match operation
                RepShareVec64         result_sh(qs);
                std::vector<uint64_t> uv_prev(1U << d), uv_next(1U << d);
                eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, query_sh, result_sh);

                // Open the resulting share vector to recover the final plaintext vector
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

        // Compute expected longest-prefix-match length using FM-index
        FMIndex  fmi(database);
        uint64_t expected_result = fmi.ComputeLPMfromWM(query);

        // Count how many zero entries in the returned result vector:
        // each zero indicates a matched prefix position
        uint64_t match_len = 0;
        for (size_t i = 0; i < result.size(); ++i) {
            if (result[i] == 0) {
                match_len++;
            }
        }

        if (match_len != expected_result) {
            throw osuCrypto::UnitTestFail(
                "SotFMI_Online_Test failed: result = " + ToString(match_len) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "SotFMI_Online_Test - Passed");
}

void OFMI_Offline_Test() {
    Logger::DebugLog(LOC, "OFMI_Offline_Test...");
    std::vector<OFMIParameters> params_list = {
        OFMIParameters(10, 10),
        // OFMIParameters(10),
        // OFMIParameters(15),
        // OFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t            d  = params.GetDatabaseBitSize();
        uint64_t            ds = params.GetDatabaseSize();
        uint64_t            qs = params.GetQuerySize();
        AdditiveSharing2P   ass(d);
        ReplicatedSharing3P rss(d);
        OFMIKeyGenerator    gen(params, ass, rss);
        FileIo              file_io;
        ShareIo             sh_io;
        KeyIo               key_io;

        // Generate keys
        std::array<OFMIKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestOFMIPath + "ofmikey_d" + ToString(d);
        key_io.SaveKey(key_path + "_0", keys[0]);
        key_io.SaveKey(key_path + "_1", keys[1]);
        key_io.SaveKey(key_path + "_2", keys[2]);

        // Generate the database and index
        std::string database = GenerateRandomString(ds - 2);
        FMIndex     fm(database);
        std::string query = GenerateRandomString(qs);
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + query);

        std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(fm);
        std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToStringMatrix());
        }

        // Save data
        std::string db_path    = kTestOFMIPath + "db_d" + ToString(d);
        std::string query_path = kTestOFMIPath + "query_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        file_io.WriteBinary(query_path, query);

        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
        }

        // Offline setup
        gen.OfflineSetUp(kTestOFMIPath);
        rss.OfflineSetUp(kTestOFMIPath + "prf");
    }
    Logger::DebugLog(LOC, "OFMI_Offline_Test - Passed");
}

void OFMI_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "OFMI_Online_Test...");
    std::vector<OFMIParameters> params_list = {
        OFMIParameters(10, 10),
        // OFMIParameters(10),
        // OFMIParameters(15),
        // OFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t qs = params.GetQuerySize();
        uint64_t nu = params.GetOWMParameters().GetOaParameters().GetParameters().GetTerminateBitsize();

        FileIo file_io;

        std::vector<uint64_t> result;
        std::string           key_path   = kTestOFMIPath + "ofmikey_d" + ToString(d);
        std::string           db_path    = kTestOFMIPath + "db_d" + ToString(d);
        std::string           query_path = kTestOFMIPath + "query_d" + ToString(d);

        std::string database;
        std::string query;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(query_path, query);

        // Factory to create a per-party task
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                // Set up replicated sharing and evaluator
                ReplicatedSharing3P rss(d);
                AdditiveSharing2P   ass_prev(d), ass_next(d);
                OFMIEvaluator       eval(params, rss, ass_prev, ass_next);
                Channels            chls(party_id, chl_prev, chl_next);

                // Load this party's key
                OFMIKey key(party_id, params);
                KeyIo   key_io_local;
                key_io_local.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load this party's shares of the database and query
                RepShareMat64 db_sh;
                RepShareMat64 query_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(party_id), query_sh);

                // Perform the PRF setup step
                eval.OnlineSetUp(party_id, kTestOFMIPath);
                rss.OnlineSetUp(party_id, kTestOFMIPath + "prf");

                // Evaluate the longest-prefix-match operation
                RepShareVec64              result_sh(qs);
                std::vector<ringoa::block> uv_prev(1U << nu), uv_next(1U << nu);
                eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, query_sh, result_sh);

                // Open the resulting share vector to recover the final plaintext vector
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

        // Compute expected longest-prefix-match length using FM-index
        FMIndex  fmi(database);
        uint64_t expected_result = fmi.ComputeLPMfromWM(query);

        // Count how many zero entries in the returned result vector:
        // each zero indicates a matched prefix position
        uint64_t match_len = 0;
        for (size_t i = 0; i < result.size(); ++i) {
            if (result[i] == 0) {
                match_len++;
            }
        }

        if (match_len != expected_result) {
            throw osuCrypto::UnitTestFail(
                "OFMI_Online_Test failed: result = " + ToString(match_len) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "OFMI_Online_Test - Passed");
}

}    // namespace test_ringoa
