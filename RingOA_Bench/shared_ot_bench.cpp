#include "shared_ot_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/protocol/shared_ot.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/utils.h"
#include "bench_common.h"

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
using ringoa::proto::SharedOtEvaluator;
using ringoa::proto::SharedOtKey;
using ringoa::proto::SharedOtKeyGenerator;
using ringoa::proto::SharedOtParameters;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64;
using ringoa::sharing::RepShareVec64;
using ringoa::sharing::RepShareView64;
using ringoa::sharing::ShareIo;

void SharedOt_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "SharedOt Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        SharedOtParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t             d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P    ass(d);
        ReplicatedSharing3P  rss(d);
        SharedOtKeyGenerator gen(params, ass);
        ShareIo              sh_io;
        KeyIo                key_io;
        TimerManager         timer_mgr;

        std::string key_path = kBenchSotPath + "sharedotkey_d" + ToString(d);
        std::string db_path  = kBenchSotPath + "db_d" + ToString(d);
        std::string idx_path = kBenchSotPath + "idx_d" + ToString(d);

        {    // KeyGen
            const std::string timer_name = "SharedOt KeyGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::array<SharedOtKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                for (size_t p = 0; p < 3; ++p)
                    key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
            }
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // OfflineSetUp
            const std::string timer_name = "SharedOt OfflineSetUp";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            rss.OfflineSetUp(kBenchSotPath + "prf");
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // DataGen
            const std::string timer_name = "SharedOt DataGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            std::vector<uint64_t> database(1ULL << d);
            for (size_t i = 0; i < database.size(); ++i)
                database[i] = i;
            uint64_t index = ass.GenerateRandomValue();
            timer_mgr.Mark("DataGen d=" + ToString(d));
            std::array<RepShareVec64, 3> database_sh = rss.ShareLocal(database);
            std::array<RepShare64, 3>    index_sh    = rss.ShareLocal(index);
            timer_mgr.Mark("ShareGen d=" + ToString(d));
            for (size_t p = 0; p < 3; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
                sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
            }
            timer_mgr.Mark("ShareSave d=" + ToString(d));
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MILLISECONDS, true);
        }
    }

    Logger::InfoLog(LOC, "SharedOt Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogSotPath + "sharedot_offline_bench", /*use_timestamp=*/true);
}

void SharedOt_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id    = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network     = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "SharedOt Online Benchmark started (repeat=" + ToString(repeat) + ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                SharedOtParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t    d        = params.GetParameters().GetInputBitsize();
                std::string key_path = kBenchSotPath + "sharedotkey_d" + ToString(d);
                std::string db_path  = kBenchSotPath + "db_d" + ToString(d);
                std::string idx_path = kBenchSotPath + "idx_d" + ToString(d);

                TimerManager timer_mgr;
                int32_t      t_setup = timer_mgr.CreateNewTimer("SharedOt OnlineSetUp " + ptag);
                int32_t      t_eval  = timer_mgr.CreateNewTimer("SharedOt Eval " + ptag);

                timer_mgr.SelectTimer(t_setup);
                timer_mgr.Start();
                ReplicatedSharing3P rss(d);
                SharedOtEvaluator   eval(params, rss);
                Channels            chls(p, chl_prev, chl_next);
                RepShare64          result_sh;
                SharedOtKey         key(p, params);
                KeyIo               key_io;
                key_io.LoadKey(key_path + "_" + ToString(p), key);
                RepShareVec64 database_sh;
                RepShare64    index_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(p), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(p), index_sh);
                std::vector<uint64_t> uv_prev(1ULL << d), uv_next(1ULL << d);
                rss.OnlineSetUp(p, kBenchSotPath + "prf");
                timer_mgr.Stop("d=" + ToString(d) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);

                timer_mgr.SelectTimer(t_eval);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(chls, key, uv_prev, uv_next, RepShareView64(database_sh), index_sh, result_sh);
                    timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                    if (i < 2)
                        Logger::InfoLog(LOC, "d=" + ToString(d) + " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
            }
        };
    };

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, MakeTask(0), MakeTask(1), MakeTask(2));
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "SharedOt Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogSotPath + "sharedot_online_p" + ToString(party_id) + "_" + network, /*use_timestamp=*/true);
}

void SharedOt_Naive_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "SharedOt Naive Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        SharedOtParameters params(db_bitsize, ringoa::fss::EvalType::kIterative);
        params.PrintParameters();
        uint64_t             d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P    ass(d);
        ReplicatedSharing3P  rss(d);
        SharedOtKeyGenerator gen(params, ass);
        ShareIo              sh_io;
        KeyIo                key_io;
        TimerManager         timer_mgr;

        std::string key_path = kBenchSotPath + "sharedot_naive_key_d" + ToString(d);
        std::string db_path  = kBenchSotPath + "db_d" + ToString(d);
        std::string idx_path = kBenchSotPath + "idx_d" + ToString(d);

        {    // KeyGen
            const std::string timer_name = "SharedOt Naive KeyGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::array<SharedOtKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                for (size_t p = 0; p < 3; ++p)
                    key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
            }
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // OfflineSetUp
            const std::string timer_name = "SharedOt Naive OfflineSetUp";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            rss.OfflineSetUp(kBenchSotPath + "prf");
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // DataGen
            const std::string timer_name = "SharedOt Naive DataGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            std::vector<uint64_t> database(1ULL << d);
            for (size_t i = 0; i < database.size(); ++i)
                database[i] = i;
            uint64_t index = ass.GenerateRandomValue();
            timer_mgr.Mark("DataGen d=" + ToString(d));
            std::array<RepShareVec64, 3> database_sh = rss.ShareLocal(database);
            std::array<RepShare64, 3>    index_sh    = rss.ShareLocal(index);
            timer_mgr.Mark("ShareGen d=" + ToString(d));
            for (size_t p = 0; p < 3; ++p) {
                sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
                sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
            }
            timer_mgr.Mark("ShareSave d=" + ToString(d));
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MILLISECONDS, true);
        }
    }

    Logger::InfoLog(LOC, "SharedOt Naive Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogSotPath + "sharedot_naive_offline_bench", /*use_timestamp=*/true);
}

void SharedOt_Naive_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id    = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network     = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "SharedOt Naive Online Benchmark started (repeat=" + ToString(repeat) + ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                SharedOtParameters params(db_bitsize, ringoa::fss::EvalType::kIterative);
                params.PrintParameters();
                uint64_t    d        = params.GetParameters().GetInputBitsize();
                std::string key_path = kBenchSotPath + "sharedot_naive_key_d" + ToString(d);
                std::string db_path  = kBenchSotPath + "db_d" + ToString(d);
                std::string idx_path = kBenchSotPath + "idx_d" + ToString(d);

                TimerManager timer_mgr;
                int32_t      t_setup = timer_mgr.CreateNewTimer("SharedOt Naive OnlineSetUp " + ptag);
                int32_t      t_eval  = timer_mgr.CreateNewTimer("SharedOt Naive Eval " + ptag);

                timer_mgr.SelectTimer(t_setup);
                timer_mgr.Start();
                ReplicatedSharing3P rss(d);
                SharedOtEvaluator   eval(params, rss);
                Channels            chls(p, chl_prev, chl_next);
                RepShare64          result_sh;
                SharedOtKey         key(p, params);
                KeyIo               key_io;
                key_io.LoadKey(key_path + "_" + ToString(p), key);
                RepShareVec64 database_sh;
                RepShare64    index_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(p), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(p), index_sh);
                std::vector<uint64_t> uv_prev(1ULL << d), uv_next(1ULL << d);
                rss.OnlineSetUp(p, kBenchSotPath + "prf");
                timer_mgr.Stop("d=" + ToString(d) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);

                timer_mgr.SelectTimer(t_eval);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(chls, key, uv_prev, uv_next, RepShareView64(database_sh), index_sh, result_sh);
                    timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                    if (i < 2)
                        Logger::InfoLog(LOC, "d=" + ToString(d) + " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
            }
        };
    };

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, MakeTask(0), MakeTask(1), MakeTask(2));
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "SharedOt Naive Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogSotPath + "sharedot_naive_online_p" + ToString(party_id) + "_" + network, /*use_timestamp=*/true);
}

}    // namespace bench_ringoa
