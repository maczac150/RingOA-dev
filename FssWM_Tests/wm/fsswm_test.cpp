#include "fsswm_test.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/fsswm.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/plain_wm.h"

namespace {

const std::string kCurrentPath   = fsswm::GetCurrentDirectory();
const std::string kTestFssWMPath = kCurrentPath + "/data/test/wm/";
const uint64_t    kFixedSeed     = 6;

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
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::ToString, fsswm::ToStringMatrix;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareVec64;
using fsswm::sharing::RepShareBlock, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::RepShareMat64, fsswm::sharing::RepShareView64;
using fsswm::sharing::RepShareMatBlock, fsswm::sharing::RepShareViewBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::FMIndex;
using fsswm::wm::FssWMEvaluator;
using fsswm::wm::FssWMKey;
using fsswm::wm::FssWMKeyGenerator;
using fsswm::wm::FssWMParameters;
using fsswm::wm::KeyIo;

void FssWM_Offline_Test() {
    Logger::DebugLog(LOC, "FssWM_Offline_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(10, 3, fsswm::fss::OutputType::kSingleBitMask),
        FssWMParameters(10, 3, fsswm::fss::OutputType::kShiftedAdditive),
        // FssWMParameters(10),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t                  d  = params.GetDatabaseBitSize();
        uint64_t                  ds = params.GetDatabaseSize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        FssWMKeyGenerator         gen(params, bss, brss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        // Generate the database and index
        std::string           database = GenerateRandomString(ds - 2);
        FMIndex               fm(database);
        std::vector<uint64_t> query    = {0, 1, 0};
        uint64_t              position = brss.GenerateRandomValue();
        Logger::DebugLog(LOC, "Database: " + database);
        Logger::DebugLog(LOC, "Query   : " + ToString(query));
        Logger::DebugLog(LOC, "Position: " + ToString(position));

        // Generate keys
        std::array<FssWMKey, 3> keys = gen.GenerateKeys();

        if (params.GetOSParameters().GetParameters().GetOutputType() == fsswm::fss::OutputType::kSingleBitMask) {
            // Save keys
            std::string key_path = kTestFssWMPath + "fsswmkeySBM_d" + ToString(d);
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);

            // Generate replicated shares for the database, query, and position
            std::array<RepShareMatBlock, 3> db_sh       = gen.GenerateDatabaseBlockShare(fm);
            std::array<RepShareVec64, 3>    query_sh    = brss.ShareLocal({0, ds - 1, 0});
            std::array<RepShare64, 3>       position_sh = brss.ShareLocal(position);
            for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
                Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
                Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToString());
                Logger::DebugLog(LOC, "Party " + ToString(p) + " position share: " + position_sh[p].ToString());
            }

            // Save data
            std::string db_path       = kTestFssWMPath + "dbSBM_d" + ToString(d);
            std::string query_path    = kTestFssWMPath + "query_d" + ToString(d);
            std::string position_path = kTestFssWMPath + "position_d" + ToString(d);

            file_io.WriteBinary(db_path, database);
            file_io.WriteBinary(query_path, query);
            file_io.WriteBinary(position_path, position);

            for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
                sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
                sh_io.SaveShare(position_path + "_" + ToString(p), position_sh[p]);
            }
        } else {
            // Save keys
            std::string key_path = kTestFssWMPath + "fsswmkeySA_d" + ToString(d);
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);

            // Generate replicated shares for the database, query, and position
            std::array<RepShareMat64, 3> db_sh       = gen.GenerateDatabaseU64Share(fm);
            std::array<RepShareVec64, 3> query_sh    = brss.ShareLocal({0, ds - 1, 0});
            std::array<RepShare64, 3>    position_sh = brss.ShareLocal(position);
            for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
                Logger::DebugLog(LOC, "Party " + ToString(p) + " rank share: " + db_sh[p].ToStringMatrix());
                Logger::DebugLog(LOC, "Party " + ToString(p) + " query share: " + query_sh[p].ToString());
                Logger::DebugLog(LOC, "Party " + ToString(p) + " position share: " + position_sh[p].ToString());
            }

            // Save data
            std::string db_path       = kTestFssWMPath + "dbSA_d" + ToString(d);
            std::string query_path    = kTestFssWMPath + "query_d" + ToString(d);
            std::string position_path = kTestFssWMPath + "position_d" + ToString(d);

            file_io.WriteBinary(db_path, database);
            file_io.WriteBinary(query_path, query);
            file_io.WriteBinary(position_path, position);

            for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
                sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
                sh_io.SaveShare(position_path + "_" + ToString(p), position_sh[p]);
            }
        }

        // Offline setup
        brss.OfflineSetUp(kTestFssWMPath + "prf");
    }
    Logger::DebugLog(LOC, "FssWM_Offline_Test - Passed");
}

void FssWM_SingleBitMask_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "FssWM_SingleBitMask_Online_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(10, 3, fsswm::fss::OutputType::kSingleBitMask),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d = params.GetDatabaseBitSize();

        FileIo file_io;

        uint64_t    result{0};
        std::string key_path      = kTestFssWMPath + "fsswmkeySBM_d" + ToString(d);
        std::string db_path       = kTestFssWMPath + "dbSBM_d" + ToString(d);
        std::string query_path    = kTestFssWMPath + "query_d" + ToString(d);
        std::string position_path = kTestFssWMPath + "position_d" + ToString(d);

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
                BinaryReplicatedSharing3P brss(d);
                FssWMEvaluator            eval(params, brss);
                Channels                  chls(party_id, chl_prev, chl_next);

                // Load this party's key
                FssWMKey key(party_id, params);
                KeyIo    key_io_local;
                key_io_local.LoadKey(key_path + "_" + ToString(party_id), key);

                // Load this party's shares of the database, query, and position
                RepShareMatBlock db_sh;
                RepShareVec64    query_sh;
                RepShare64       position_sh;
                ShareIo          sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(party_id), query_sh);
                sh_io.LoadShare(position_path + "_" + ToString(party_id), position_sh);

                // Perform the PRF setup step
                brss.OnlineSetUp(party_id, kTestFssWMPath + "prf");

                // Evaluate the rank operation
                RepShare64 result_sh;
                eval.EvaluateRankCF_SingleBitMask(chls, key, db_sh, RepShareView64(query_sh), position_sh, result_sh);

                // Open the resulting share to recover the final value
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

        // Verify against the plain-wavelet-matrix rank computation
        FMIndex  fmi(database);
        uint64_t expected_result = fmi.GetWaveletMatrix().RankCF(2, position);
        if (result != expected_result) {
            throw osuCrypto::UnitTestFail(
                "FssWM_Online_Test failed: result = " + ToString(result) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "FssWM_SingleBitMask_Online_Test - Passed");
}

void FssWM_ShiftedAdditive_Online_Test(const osuCrypto::CLP &cmd) {
    Logger::DebugLog(LOC, "FssWM_ShiftedAdditive_Online_Test...");
    std::vector<FssWMParameters> params_list = {
        FssWMParameters(10, 3, fsswm::fss::OutputType::kShiftedAdditive),
        // FssWMParameters(15),
        // FssWMParameters(20),
    };

    for (const auto &params : params_list) {
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseBitSize();
        uint64_t nu = params.GetOSParameters().GetParameters().GetTerminateBitsize();

        FileIo file_io;

        uint64_t    result{0};
        std::string key_path      = kTestFssWMPath + "fsswmkeySA_d" + ToString(d);
        std::string db_path       = kTestFssWMPath + "dbSA_d" + ToString(d);
        std::string query_path    = kTestFssWMPath + "query_d" + ToString(d);
        std::string position_path = kTestFssWMPath + "position_d" + ToString(d);

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
                BinaryReplicatedSharing3P brss(d);
                FssWMEvaluator            eval(params, brss);
                Channels                  chls(party_id, chl_prev, chl_next);

                // Load this party's key
                FssWMKey key(party_id, params);
                KeyIo    key_io_local;
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
                brss.OnlineSetUp(party_id, kTestFssWMPath + "prf");

                // Evaluate the rank operation
                RepShare64                result_sh;
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
                eval.EvaluateRankCF_ShiftedAdditive(chls, key, uv_prev, uv_next, db_sh, RepShareView64(query_sh), position_sh, result_sh);

                // Open the resulting share to recover the final value
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

        // Verify against the plain-wavelet-matrix rank computation
        FMIndex  fmi(database);
        uint64_t expected_result = fmi.GetWaveletMatrix().RankCF(2, position);
        if (result != expected_result) {
            throw osuCrypto::UnitTestFail(
                "FssWM_Online_Test failed: result = " + ToString(result) +
                ", expected = " + ToString(expected_result));
        }
    }

    Logger::DebugLog(LOC, "FssWM_ShiftedAdditive_Online_Test - Passed");
}

}    // namespace test_fsswm
