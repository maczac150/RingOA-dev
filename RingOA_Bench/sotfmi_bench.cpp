#include "sotfmi_bench.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/fm_index/sotfmi.h"
#include "RingOA/protocol/key_io.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/utils.h"
#include "RingOA/wm/plain_wm.h"
#include "bench_common.h"

namespace {

const uint64_t    kFixedSeed       = 6;

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

constexpr ringoa::fss::EvalType kEvalType = ringoa::fss::EvalType::kHybridBatched;

}    // namespace

namespace bench_ringoa {

using ringoa::Channels;
using ringoa::CreateSequence;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::ThreePartyNetworkManager;
using ringoa::TimerManager;
using ringoa::ToString;
using ringoa::fm_index::SotFMIEvaluator;
using ringoa::fm_index::SotFMIKey;
using ringoa::fm_index::SotFMIKeyGenerator;
using ringoa::fm_index::SotFMIParameters;
using ringoa::proto::KeyIo;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64;
using ringoa::sharing::RepShareMat64;
using ringoa::sharing::RepShareVec64;
using ringoa::sharing::ShareIo;
using ringoa::wm::FMIndex;

void SotFMI_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat        = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> text_bitsizes = SelectBitsizes(cmd);
    std::vector<uint64_t> query_sizes   = SelectQueryBitsize(cmd);

    Logger::InfoLog(LOC, "SotFMI Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            SotFMIParameters params(text_bitsize, query_size, 3, kEvalType);
            params.PrintParameters();

            uint64_t d  = params.GetDatabaseBitSize();
            uint64_t ds = params.GetDatabaseSize();
            uint64_t qs = params.GetQuerySize();

            AdditiveSharing2P   ass(d);
            ReplicatedSharing3P rss(d);
            SotFMIKeyGenerator  gen(params, ass, rss);
            ShareIo             sh_io;
            KeyIo               key_io;
            TimerManager        timer_mgr;

            std::string key_path   = kBenchSotfmiPath + "sotfmikey_d" + ToString(d) + "_qs" + ToString(qs);
            std::string db_path    = kBenchSotfmiPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
            std::string query_path = kBenchSotfmiPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

            {    // KeyGen
                const std::string timer_name = "SotFMI KeyGen";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    std::array<SotFMIKey, 3> keys = gen.GenerateKeys();
                    timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=" + ToString(i));
                    for (size_t p = 0; p < 3; ++p)
                        key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
                }
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
            }

            {    // OfflineSetUp
                const std::string timer_name = "SotFMI OfflineSetUp";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                timer_mgr.Start();
                rss.OfflineSetUp(kBenchSotfmiPath + "prf");
                timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
            }

            {    // DataGen
                const std::string timer_name = "SotFMI DataGen";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                timer_mgr.Start();
                std::string database = GenerateRandomString(ds - 2);
                std::string query    = GenerateRandomString(qs);
                timer_mgr.Mark("DataGen d=" + ToString(d) + " qs=" + ToString(qs));
                FMIndex fm(database);
                timer_mgr.Mark("FMIndex d=" + ToString(d) + " qs=" + ToString(qs));
                std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(fm);
                std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
                timer_mgr.Mark("ShareGen d=" + ToString(d) + " qs=" + ToString(qs));
                for (size_t p = 0; p < 3; ++p) {
                    sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
                    sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
                }
                timer_mgr.Mark("ShareSave d=" + ToString(d) + " qs=" + ToString(qs));
                timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MILLISECONDS, true);
            }
        }
    }

    Logger::InfoLog(LOC, "SotFMI Offline Benchmark completed");
    if (kEvalType == ringoa::fss::EvalType::kHybridBatched) {
        Logger::ExportLogListAndClear(kLogSotfmiPath + "sotfmi_offline", true);
    } else {
        Logger::ExportLogListAndClear(kLogSotfmiPath + "sotfmi_naive_offline", true);
    }
}

void SotFMI_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat        = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id      = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network       = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> text_bitsizes = SelectBitsizes(cmd);
    std::vector<uint64_t> query_sizes   = SelectQueryBitsize(cmd);

    Logger::InfoLog(LOC, "SotFMI Online Benchmark started (repeat=" + ToString(repeat) + ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto text_bitsize : text_bitsizes) {
                for (auto query_size : query_sizes) {
                    SotFMIParameters params(text_bitsize, query_size, 3, kEvalType);
                    params.PrintParameters();

                    uint64_t d  = params.GetDatabaseBitSize();
                    uint64_t qs = params.GetQuerySize();

                    std::string key_path   = kBenchSotfmiPath + "sotfmikey_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string db_path    = kBenchSotfmiPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string query_path = kBenchSotfmiPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

                    TimerManager timer_mgr;
                    int32_t      id_setup = timer_mgr.CreateNewTimer("SotFMI OnlineSetUp " + ptag);
                    int32_t      id_eval  = timer_mgr.CreateNewTimer("SotFMI Eval " + ptag);

                    timer_mgr.SelectTimer(id_setup);
                    timer_mgr.Start();
                    ReplicatedSharing3P   rss(d);
                    AdditiveSharing2P     ass(d);
                    SotFMIEvaluator       eval(params, rss, ass);
                    Channels              chls(p, chl_prev, chl_next);
                    std::vector<uint64_t> uv_prev(1ULL << d), uv_next(1ULL << d);
                    SotFMIKey             key(p, params);
                    KeyIo                 key_io;
                    key_io.LoadKey(key_path + "_" + ToString(p), key);
                    RepShareMat64 db_sh;
                    RepShareMat64 query_sh;
                    ShareIo       sh_io;
                    sh_io.LoadShare(db_path + "_" + ToString(p), db_sh);
                    sh_io.LoadShare(query_path + "_" + ToString(p), query_sh);
                    rss.OnlineSetUp(p, kBenchSotfmiPath + "prf");
                    timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                    timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);

                    timer_mgr.SelectTimer(id_eval);
                    for (uint64_t i = 0; i < repeat; ++i) {
                        timer_mgr.Start();
                        RepShareVec64 result_sh(qs);
                        eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, query_sh, result_sh);
                        timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=" + ToString(i));
                        if (i < 2)
                            Logger::InfoLog(LOC, "d=" + ToString(d) + " qs=" + ToString(qs) + " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                        chls.ResetStats();
                    }
                    timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
                }
            }
        };
    };

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, MakeTask(0), MakeTask(1), MakeTask(2));
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "SotFMI Online Benchmark completed");
    if (kEvalType == ringoa::fss::EvalType::kHybridBatched) {
        Logger::ExportLogListAndClear(kLogSotfmiPath + "sotfmi_online_p" + ToString(party_id) + "_" + network, true);
    } else {
        Logger::ExportLogListAndClear(kLogSotfmiPath + "sotfmi_naive_online_p" + ToString(party_id) + "_" + network, true);
    }
}

}    // namespace bench_ringoa
