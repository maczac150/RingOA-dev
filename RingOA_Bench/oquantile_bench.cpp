#include "oquantile_bench.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/utils.h"
#include "RingOA/wm/oquantile.h"
#include "RingOA/wm/plain_wm.h"

namespace {

const std::string kCurrentPath        = ringoa::GetCurrentDirectory();
const std::string kBenchOQuantilePath = kCurrentPath + "/data/bench/wm/";
const uint64_t    kFixedSeed          = 6;

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

// std::vector<uint64_t> db_bitsizes = {16, 18, 20, 22, 24, 26, 28};
std::vector<uint64_t> db_bitsizes = ringoa::CreateSequence(10, 11);

constexpr uint64_t kIterDefault = 10;

}    // namespace

namespace bench_ringoa {

using ringoa::block;
using ringoa::Channels;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::Mod2N;
using ringoa::ThreePartyNetworkManager;
using ringoa::TimerManager;
using ringoa::ToString, ringoa::Format;
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

void OQuantile_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OQuantile_Offline_Bench...");
    uint64_t iter = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;

    for (auto db_bitsize : db_bitsizes) {
        OQuantileParameters params(db_bitsize);
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

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("OQuantile KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("OQuantile OfflineSetUp");

        std::string key_path = kBenchOQuantilePath + "oquantilekey_d" + ToString(d);
        std::string db_path  = kBenchOQuantilePath + "db_d" + ToString(d);
        std::string idx_path = kBenchOQuantilePath + "query_d" + ToString(d);

        for (uint64_t i = 0; i < iter; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::array<OQuantileKey, 3> keys = gen.GenerateKeys();
            timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d));

            // Save keys
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);
        }
        // Offline setup
        timer_mgr.SelectTimer(timer_off);
        timer_mgr.Start();
        gen.OfflineSetUp(kBenchOQuantilePath);
        rss.OfflineSetUp(kBenchOQuantilePath + "prf");
        timer_mgr.Stop("OfflineSetUp(0) d=" + ToString(d));
        timer_mgr.PrintAllResults("Gen d=" + ToString(d), ringoa::MICROSECONDS, true);

        // Generate the database and index
        int32_t timer_data = timer_mgr.CreateNewTimer("OS DataGen");
        timer_mgr.SelectTimer(timer_data);
        timer_mgr.Start();
        std::vector<uint64_t> database = GenerateRandomVector(ds - 1, params.GetSigma());
        std::vector<uint64_t> query    = {/* left = */ 123, /* right = */ 456, /* k = */ 100};
        WaveletMatrix         wm(database, params.GetSigma());
        timer_mgr.Mark("DataGen d=" + ToString(d));

        std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(wm);
        std::array<RepShareVec64, 3> query_sh = rss.ShareLocal(query);
        timer_mgr.Mark("ShareGen d=" + ToString(d));

        // Save data
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            sh_io.SaveShare(idx_path + "_" + ToString(p), query_sh[p]);
        }
        timer_mgr.Mark("ShareSave d=" + ToString(d));
        timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), ringoa::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "OQuantile_Offline_Bench - Finished");
    // Logger::ExportLogList("./data/logs/oquantile_offline_bench");
}

void OQuantile_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OQuantile_Online_Bench...");
    uint64_t    iter     = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Helper that returns a task lambda for a given party_id
    auto MakeTask = [&](int party_id) {
        // Capture d, nu, params, and paths
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                OQuantileParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t d  = params.GetDatabaseBitSize();
                uint64_t s  = params.GetShareSize();
                uint64_t nu = params.GetOaParameters().GetParameters().GetTerminateBitsize();

                std::string key_path   = kBenchOQuantilePath + "oquantilekey_d" + ToString(d);
                std::string db_path    = kBenchOQuantilePath + "db_d" + ToString(d);
                std::string query_path = kBenchOQuantilePath + "query_d" + ToString(d);

                // (1) Set up TimerManager and timers
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("RingOA SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("RingOA Eval");

                // (2) Begin SetUp timing
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                // (3) Set up the replicated-sharing object and evaluator
                ReplicatedSharing3P rss(s);
                AdditiveSharing2P   ass_prev(s), ass_next(s);
                OQuantileEvaluator  eval(params, rss, ass_prev, ass_next);
                Channels            chls(party_id, chl_prev, chl_next);
                RepShare64          result_sh;

                // (4) Load this party’s key
                OQuantileKey key(party_id, params);
                KeyIo        key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (5) Load this party’s shares of both databases and the index
                RepShareMat64 db_sh;
                RepShareVec64              query_sh;
                std::vector<ringoa::block> uv_prev(1U << nu), uv_next(1U << nu);
                ShareIo                    sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(party_id), query_sh);
                RepShare64 left_sh = query_sh.At(0), right_sh = query_sh.At(1), k_sh = query_sh.At(2);

                // (6) Setup PRF keys
                eval.OnlineSetUp(party_id, kBenchOQuantilePath);
                rss.OnlineSetUp(party_id, kBenchOQuantilePath + "prf");

                // (7) Stop SetUp timer
                timer_mgr.Stop("SetUp d=" + ToString(d));

                // (8) Begin Eval timing
                timer_mgr.SelectTimer(timer_eval);

                // (9) iter Evaluate and measure each iteration
                for (uint64_t i = 0; i < iter; ++i) {
                    timer_mgr.Start();
                    eval.EvaluateQuantile_Parallel(chls, key, uv_prev, uv_next,
                                                   db_sh, left_sh, right_sh, k_sh, result_sh);
                    timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));

                    if (i < 2)
                        Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }

                // (10) Print all timing results
                timer_mgr.PrintAllResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
            }
        };
    };

    // Create tasks for parties 0, 1, and 2
    auto task0 = MakeTask(0);
    auto task1 = MakeTask(1);
    auto task2 = MakeTask(2);

    // Configure network based on party ID and wait for completion
    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task0, task1, task2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OQuantile_Online_Bench - Finished");
    // Logger::ExportLogList("./data/logs/oquantile_online_p" + ToString(party_id) + "_" + network);
}

}    // namespace bench_ringoa
