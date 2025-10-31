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
#include "bench_common.h"

namespace {

const uint64_t kFixedSeed = 6;

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
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OQuantile Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        OQuantileParameters params(db_bitsize);
        params.PrintParameters();

        const uint64_t d  = params.GetDatabaseBitSize();
        const uint64_t s  = params.GetShareSize();
        const uint64_t ds = params.GetDatabaseSize();

        AdditiveSharing2P     ass(s);
        ReplicatedSharing3P   rss(s);
        OQuantileKeyGenerator gen(params, ass, rss);
        FileIo                file_io;
        ShareIo               sh_io;
        KeyIo                 key_io;
        TimerManager          timer_mgr;

        std::string key_path = kBenchWmPath + "oquantilekey_d" + ToString(d);
        std::string db_path  = kBenchWmPath + "db_d" + ToString(d);
        std::string idx_path = kBenchWmPath + "query_d" + ToString(d);

        // 1) KeyGen (repeat times)
        {
            const std::string timer_name = "OQuantile KeyGen";
            const int32_t     timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::array<OQuantileKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));

                // Persist keys for each party
                key_io.SaveKey(key_path + "_0", keys[0]);
                key_io.SaveKey(key_path + "_1", keys[1]);
                key_io.SaveKey(key_path + "_2", keys[2]);
            }
            timer_mgr.PrintCurrentResults(
                "d=" + ToString(d),
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }

        // 2) OfflineSetUp (measured once per d)
        {
            const std::string timer_name = "OQuantile OfflineSetUp";
            const int32_t     timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            timer_mgr.Start();
            gen.OfflineSetUp(kBenchWmPath);
            rss.OfflineSetUp(kBenchWmPath + "prf");
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");

            timer_mgr.PrintCurrentResults(
                "d=" + ToString(d),
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }

        // 3) Data generation + secret sharing (once per d)
        {
            const std::string timer_name = "OQuantile DataGen";
            const int32_t     timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            timer_mgr.Start();

            // Build random database of size ds over [0, sigma)
            std::vector<uint64_t> database = GenerateRandomVector(ds, params.GetSigma());

            // Example query (left, right, k). Replace with your actual generator as needed.
            std::vector<uint64_t> query = {/* left = */ 123, /* right = */ 456, /* k = */ 100};

            // Build Wavelet Matrix for the dataset
            WaveletMatrix wm(database, params.GetSigma());
            timer_mgr.Mark("DataGen d=" + ToString(d));

            // Secret-share database and query
            std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(wm);
            std::array<RepShareVec64, 3> query_sh = rss.ShareLocal(query);
            timer_mgr.Mark("ShareGen d=" + ToString(d));

            // Save per-party shares
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
                sh_io.SaveShare(idx_path + "_" + ToString(p), query_sh[p]);
            }
            timer_mgr.Mark("ShareSave d=" + ToString(d));

            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults(
                "d=" + ToString(d),
                ringoa::TimeUnit::MILLISECONDS,
                /*show_details=*/true);
        }
    }

    Logger::InfoLog(LOC, "OQuantile Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogWmPath + "oquantile_offline_bench", /*use_timestamp=*/true);
}

void OQuantile_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id    = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network     = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OQuantile Online Benchmark started (repeat=" + ToString(repeat) +
                             ", party=" + ToString(party_id) + ")");

    // Helper that returns a task lambda for a given party p
    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";

        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                // ----- Parameters -----
                OQuantileParameters params(db_bitsize);
                params.PrintParameters();

                const uint64_t d  = params.GetDatabaseBitSize();
                const uint64_t s  = params.GetShareSize();
                const uint64_t nu = params.GetOaParameters().GetParameters().GetTerminateBitsize();

                std::string key_path   = kBenchWmPath + std::string("oquantilekey_d") + ToString(d);
                std::string db_path    = kBenchWmPath + std::string("db_d") + ToString(d);
                std::string query_path = kBenchWmPath + std::string("query_d") + ToString(d);

                // ----- Timers -----
                TimerManager      timer_mgr;
                const std::string timer_setup_name = "OQuantile OnlineSetUp " + ptag;
                const std::string timer_eval_name  = "OQuantile Eval " + ptag;
                const int32_t     timer_setup      = timer_mgr.CreateNewTimer(timer_setup_name);
                const int32_t     timer_eval       = timer_mgr.CreateNewTimer(timer_eval_name);

                // ================================
                // OnlineSetUp timing
                // ================================
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                ReplicatedSharing3P rss(s);
                AdditiveSharing2P   ass_prev(s), ass_next(s);
                OQuantileEvaluator  eval(params, rss, ass_prev, ass_next);
                Channels            chls(p, chl_prev, chl_next);
                RepShare64          result_sh;

                // Load key for this party
                OQuantileKey key(p, params);
                KeyIo        key_io;
                key_io.LoadKey(key_path + "_" + ToString(p), key);

                // Load shares (database and query = [left,right,k])
                RepShareMat64 db_sh;
                RepShareVec64 query_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(p), db_sh);
                sh_io.LoadShare(query_path + "_" + ToString(p), query_sh);

                // Extract query components
                RepShare64 left_sh  = query_sh.At(0);
                RepShare64 right_sh = query_sh.At(1);
                RepShare64 k_sh     = query_sh.At(2);

                // Buffers sized by terminate bitsize
                std::vector<ringoa::block> uv_prev(1ULL << nu), uv_next(1ULL << nu);

                // PRF / evaluator setup
                eval.OnlineSetUp(p, kBenchWmPath);
                rss.OnlineSetUp(p, kBenchWmPath + "prf");

                timer_mgr.Stop("d=" + ToString(d) + " iter=0");
                timer_mgr.PrintCurrentResults(
                    "d=" + ToString(d),
                    ringoa::TimeUnit::MICROSECONDS,
                    /*show_details=*/true);

                // ================================
                // Eval timing
                // ================================
                timer_mgr.SelectTimer(timer_eval);

                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.EvaluateQuantile_Parallel(
                        chls, key, uv_prev, uv_next,
                        db_sh, left_sh, right_sh, k_sh, result_sh);
                    timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));

                    if (i < 2) {
                        Logger::InfoLog(LOC, "d=" + ToString(d) +
                                                 " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                    }
                    chls.ResetStats();
                }

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.PrintCurrentResults(
                    "d=" + ToString(d),
                    ringoa::TimeUnit::MICROSECONDS,
                    /*show_details=*/true);
            }
        };
    };

    // Create tasks for parties 0, 1, and 2
    auto task0 = MakeTask(0);
    auto task1 = MakeTask(1);
    auto task2 = MakeTask(2);

    // Configure network and run
    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task0, task1, task2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OQuantile Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogWmPath + "oquantile_online_p" + ToString(party_id) + "_" + network,
                                  /*use_timestamp=*/true);
}

}    // namespace bench_ringoa
