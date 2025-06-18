#include "fssfmi_test.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/fm_index/fssfmi.h"
#include "FssWM/protocol/key_io.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/plain_wm.h"

namespace {

const std::string kCurrentPath    = fsswm::GetCurrentDirectory();
const std::string kTestFssFMIPath = kCurrentPath + "/data/test/fmi/";
const uint64_t    kFixedSeed      = 6;

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

namespace test_fsswm {

using fsswm::Channels;
using fsswm::CreateSequence;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString;
using fsswm::fm_index::FssFMIEvaluator;
using fsswm::fm_index::FssFMIKey;
using fsswm::fm_index::FssFMIKeyGenerator;
using fsswm::fm_index::FssFMIParameters;
using fsswm::proto::KeyIo;
using fsswm::sharing::AdditiveSharing2P, fsswm::sharing::BinarySharing2P;
using fsswm::sharing::ReplicatedSharing3P, fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareBlock;
using fsswm::sharing::RepShareMat64, fsswm::sharing::RepShareMatBlock;
using fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::FMIndex;

void FssFMI_Offline_Test() {
    Logger::DebugLog(LOC, "FssFMI_Offline_Test...");
    std::vector<FssFMIParameters> params_list = {
        FssFMIParameters(10, 10),
        // FssFMIParameters(10),
        // FssFMIParameters(15),
        // FssFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d  = params.GetDatabaseBitSize();
        uint64_t                  ds = params.GetDatabaseSize();
        uint64_t                  qs = params.GetQuerySize();
        AdditiveSharing2P         ass(d);
        BinarySharing2P           bss(d);
        ReplicatedSharing3P       rss(d);
        BinaryReplicatedSharing3P brss(d);
        FssFMIKeyGenerator        gen(params, ass, bss, rss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate keys
        std::array<FssFMIKey, 3> keys = gen.GenerateKeys();

        // Save keys
        std::string key_path = kTestFssFMIPath + "fssfmikey_d" + ToString(d);
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
        std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryShare(fm, query);
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
            Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToStringMatrix());
        }

        // Save data
        std::string db_path    = kTestFssFMIPath + "db_d" + ToString(d);
        std::string query_path = kTestFssFMIPath + "query_d" + ToString(d);

        file_io.WriteBinary(db_path, database);
        file_io.WriteBinary(query_path, query);

        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
        }

        // Offline setup
        gen.OfflineSetUp(kTestFssFMIPath);
        rss.OfflineSetUp(kTestFssFMIPath + "prf");
        brss.OfflineSetUp(kTestFssFMIPath + "prf_bin");
    }
    Logger::DebugLog(LOC, "FssFMI_Offline_Test - Passed");
}

void FssFMI_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "FssFMI_Online_Test...");
    std::vector<FssFMIParameters> params_list = {
        FssFMIParameters(10, 10),
        // FssFMIParameters(10),
        // FssFMIParameters(15),
        // FssFMIParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t qs = params.GetQuerySize();
        uint64_t nu = params.GetFssWMParameters().GetOSParameters().GetParameters().GetTerminateBitsize();

        FileIo file_io;

        std::vector<uint64_t> result;
        std::string           key_path   = kTestFssFMIPath + "fssfmikey_d" + ToString(d);
        std::string           db_path    = kTestFssFMIPath + "db_d" + ToString(d);
        std::string           query_path = kTestFssFMIPath + "query_d" + ToString(d);

        std::string database;
        std::string query;
        file_io.ReadBinary(db_path, database);
        file_io.ReadBinary(query_path, query);

        // Factory to create a per-party task
        auto MakeTask = [&](int party_id) {
            return [=, &result](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                // Set up replicated sharing and evaluator
                ReplicatedSharing3P       rss(d);
                BinaryReplicatedSharing3P brss(d);
                AdditiveSharing2P         ass_prev(d), ass_next(d);
                FssFMIEvaluator           eval(params, rss, brss, ass_prev, ass_next);
                Channels                  chls(party_id, chl_prev, chl_next);

                // Load this party's key
                FssFMIKey key(party_id, params);
                KeyIo     key_io_local;
                key_io_local.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load this party's shares of the database and query
                RepShareMat64 db_sh;
                RepShareMat64 query_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(party_id), query_sh);

                // Perform the PRF setup step
                eval.OnlineSetUp(party_id, kTestFssFMIPath);
                rss.OnlineSetUp(party_id, kTestFssFMIPath + "prf");
                brss.OnlineSetUp(party_id, kTestFssFMIPath + "prf_bin");

                // Evaluate the longest-prefix-match operation
                RepShareVec64             result_sh(qs);
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
                eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, query_sh, result_sh);

                // Open the resulting share vector to recover the final plaintext vector
                brss.Open(chls, result_sh, result);
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
                "FssFMI_Online_Test failed: result = " + ToString(match_len) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "FssFMI_Online_Test - Passed");
}

}    // namespace test_fsswm
