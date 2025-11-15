#include "oquantile_bench.h"

#include <charconv>
#include <cryptoTools/Common/TestCollection.h>
#include <random>

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

void LoadVAFValues(const std::string &file_path, std::vector<uint64_t> &values) {
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }

    const size_t capacity = values.size();    // fixed capacity
    size_t       idx      = 0;

    std::string line;
    line.reserve(32);

    while (idx < capacity && std::getline(ifs, line)) {
        uint64_t v     = 0;
        auto     begin = line.data();
        auto     end   = line.data() + line.size();

        auto result = std::from_chars(begin, end, v);
        if (result.ec != std::errc()) {
            throw std::runtime_error("Invalid integer in line: " + line);
        }

        values[idx++] = v;
    }

    // Fill unused tail with 0
    while (idx < capacity) {
        values[idx++] = 0;
    }
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

        std::string key_path   = kBenchWmPath + "oquantilekey_d" + ToString(d);
        std::string db_path    = kBenchWmPath + "db_d" + ToString(d);
        std::string query_path = kBenchWmPath + "query_d" + ToString(d);

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
                sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
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

void OQuantile_VAF_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t    repeat   = cmd.getOr("repeat", kRepeatDefault);
    std::string vaf_file = cmd.getOr("vaf_file", kVafDataPath + "vaf_values.txt");

    Logger::InfoLog(LOC, "OQuantile VAF Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    // VAF database size is approximately 23 million entries < 2^25
    // sigma for VAF values is 7 (0-100)
    OQuantileParameters params(25, 7);
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

    std::string key_path    = kBenchWmPath + "oquantilekey_d" + ToString(d);
    std::string db_path     = kBenchWmPath + "db_vaf_d" + ToString(d);
    std::string query1_path = kBenchWmPath + "query_brca1_d" + ToString(d);
    std::string query2_path = kBenchWmPath + "query_brca2_d" + ToString(d);

    // 1) KeyGen (repeat times)
    {
        const std::string timer_name = "OQuantile VAF KeyGen";
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
        const std::string timer_name = "OQuantile VAF OfflineSetUp";
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
        const std::string timer_name = "OQuantile VAF DataGen";
        const int32_t     timer_id   = timer_mgr.CreateNewTimer(timer_name);
        timer_mgr.SelectTimer(timer_id);

        timer_mgr.Start();

        // Build database of size ds over [0, sigma)
        std::vector<uint64_t> database(ds - 1);
        LoadVAFValues(vaf_file, database);
        Logger::InfoLog(LOC, "Loaded VAF data from file: " + ToString(database));

        // Build Wavelet Matrix once
        WaveletMatrix wm(database, params.GetSigma());
        timer_mgr.Mark("DataGen d=" + ToString(d));

        // ======================================================
        // Secret-share the database ONCE
        // ======================================================
        std::array<RepShareMat64, 3> db_sh = gen.GenerateDatabaseU64Share(wm);
        timer_mgr.Mark("ShareDB d=" + ToString(d));

        // Save database shares (common for both queries)
        for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
        }

        // ======================================================
        // BRCA1 Query
        // ======================================================
        {
            uint64_t left1  = 19613831;
            uint64_t right1 = 19614213;
            uint64_t count1 = right1 - left1 - 1;    // max/rank-"last"

            std::vector<uint64_t> q1 = {left1, right1, count1};

            // Secret-share BRCA1 query
            std::array<RepShareVec64, 3> q1_sh = rss.ShareLocal(q1);

            // Save BRCA1 query shares
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(query1_path + "_" + ToString(p), q1_sh[p]);
            }
        }

        // ======================================================
        // BRCA2 Query
        // ======================================================
        {
            uint64_t left2  = 16705359;
            uint64_t right2 = 16705667;
            uint64_t count2 = right2 - left2 - 1;    // max/rank-"last"

            std::vector<uint64_t> q2 = {left2, right2, count2};

            // Secret-share BRCA2 query
            std::array<RepShareVec64, 3> q2_sh = rss.ShareLocal(q2);

            // Save BRCA2 query shares
            for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                sh_io.SaveShare(query2_path + "_" + ToString(p), q2_sh[p]);
            }
        }

        timer_mgr.Mark("ShareSave d=" + ToString(d));
        timer_mgr.Stop("d=" + ToString(d) + " iter=0");

        timer_mgr.PrintCurrentResults(
            "d=" + ToString(d),
            ringoa::TimeUnit::MILLISECONDS,
            /*show_details=*/true);
    }

    Logger::InfoLog(LOC, "OQuantile VAF Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogWmPath + "oquantile_vaf_offline_bench", /*use_timestamp=*/true);
}

void OQuantile_VAF_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t    repeat   = cmd.getOr("repeat", kRepeatDefault);
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    uint64_t    qid      = cmd.getOr("qid", 1);    // query id: 1 (BRCA1) or 2 (BRCA2)
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    Logger::InfoLog(LOC, "OQuantile VAF Online Benchmark started (repeat=" + ToString(repeat) +
                             ", party=" + ToString(party_id) + ")");

    // Helper that returns a task lambda for a given party p
    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";

        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            // ----- Parameters -----
            OQuantileParameters params(25, 7);
            params.PrintParameters();

            const uint64_t d  = params.GetDatabaseBitSize();
            const uint64_t s  = params.GetShareSize();
            const uint64_t nu = params.GetOaParameters().GetParameters().GetTerminateBitsize();

            std::string key_path    = kBenchWmPath + std::string("oquantilekey_d") + ToString(d);
            std::string db_path     = kBenchWmPath + std::string("db_vaf_d") + ToString(d);
            std::string query1_path = kBenchWmPath + "query_brca1_d" + ToString(d);
            std::string query2_path = kBenchWmPath + "query_brca2_d" + ToString(d);

            // ----- Timers -----
            TimerManager      timer_mgr;
            const std::string timer_setup_name = "OQuantile VAF OnlineSetUp " + ptag;
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
            sh_io.LoadShare((qid == 1 ? query1_path : query2_path) + "_" + ToString(p), query_sh);

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
                // Extract query components
                RepShare64 left_sh  = query_sh.At(0);
                RepShare64 right_sh = query_sh.At(1);
                RepShare64 k_sh     = query_sh.At(2);
                timer_mgr.Start();
                eval.EvaluateQuantile_Parallel(
                    chls, key, uv_prev, uv_next,
                    db_sh, left_sh, right_sh, k_sh, result_sh);
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));

                // rss.Open(chls, result_sh, result);
                // Logger::InfoLog(LOC, ptag + " d=" + ToString(d) + " iter=" + ToString(i) +
                //                          " Quantile Result=" + ToString(result));

                if (i < 2) {
                    Logger::InfoLog(LOC, "d=" + ToString(d) +
                                             " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                }
                chls.ResetStats();
                ass_prev.ResetTripleIndex();
                ass_next.ResetTripleIndex();
            }

            timer_mgr.SelectTimer(timer_eval);
            timer_mgr.PrintCurrentResults(
                "d=" + ToString(d),
                ringoa::TimeUnit::MILLISECONDS,
                /*show_details=*/true);
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

    Logger::InfoLog(LOC, "OQuantile VAF Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogWmPath + "oquantile_vaf_brca" + ToString(qid) + "_online_p" + ToString(party_id) + "_" + network,
                                  /*use_timestamp=*/false);
}

}    // namespace bench_ringoa
