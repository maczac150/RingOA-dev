#include "obliv_select_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/protocol/obliv_select.h"
#include "RingOA/sharing/binary_2p.h"
#include "RingOA/sharing/binary_3p.h"
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
using ringoa::fss::dpf::DpfEvaluator;
using ringoa::fss::dpf::DpfKey;
using ringoa::fss::dpf::DpfKeyGenerator;
using ringoa::fss::dpf::DpfParameters;
using ringoa::proto::KeyIo;
using ringoa::proto::OblivSelectEvaluator;
using ringoa::proto::OblivSelectKey;
using ringoa::proto::OblivSelectKeyGenerator;
using ringoa::proto::OblivSelectParameters;
using ringoa::sharing::BinaryReplicatedSharing3P;
using ringoa::sharing::BinarySharing2P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareBlock;
using ringoa::sharing::RepShareVec64, ringoa::sharing::RepShareVecBlock;
using ringoa::sharing::RepShareView64, ringoa::sharing::RepShareViewBlock;
using ringoa::sharing::ShareIo;

void OblivSelect_ComputeDotProductBlockSIMD_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OblivSelect ComputeDotProductBlockSIMD Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters     params(db_bitsize);
        uint64_t                  n = params.GetParameters().GetInputBitsize();
        DpfKeyGenerator           gen(params.GetParameters());
        BinaryReplicatedSharing3P brss(n);
        OblivSelectEvaluator      eval_os(params, brss);

        uint64_t alpha = brss.GenerateRandomValue();
        uint64_t beta  = 1;

        RepShareVecBlock database_sh(1U << n);
        for (size_t i = 0; i < database_sh.Size(); ++i) {
            database_sh[0][i] = ringoa::MakeBlock(0, i);
            database_sh[1][i] = ringoa::MakeBlock(0, i);
        }
        uint64_t pr_prev = brss.GenerateRandomValue();
        uint64_t pr_next = brss.GenerateRandomValue();

        // Generate keys (outside per-iteration timing)
        std::pair<DpfKey, DpfKey> keys_next = gen.GenerateKeys(alpha, beta);
        std::pair<DpfKey, DpfKey> keys_prev = gen.GenerateKeys(alpha, beta);

        // --- Timer setup ---
        const std::string timer_name = "OblivSelect ComputeDotProductBlockSIMD";
        TimerManager      timer_mgr;
        int32_t           timer_id = timer_mgr.CreateNewTimer(timer_name);
        timer_mgr.SelectTimer(timer_id);

        // --- Measurement loop ---
        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            block result_prev = eval_os.ComputeDotProductBlockSIMD(keys_prev.first, database_sh[0], pr_prev);
            block result_next = eval_os.ComputeDotProductBlockSIMD(keys_next.second, database_sh[1], pr_next);
            timer_mgr.Stop(
                "n=" + ToString(n) +
                " algo=OS-ComputeDotProductBlockSIMD" +
                " iter=" + ToString(i));

            // Optional: reduce log noise; switch to Debug if you prefer
            Logger::DebugLog(LOC, "Result Prev: " + Format(result_prev) + ", Result Next: " + Format(result_next));
        }

        // --- Summarize results ---
        const std::string summary_msg =
            "n=" + ToString(n) +
            " algo=OS-ComputeDotProductBlockSIMD";

        timer_mgr.PrintCurrentResults(
            summary_msg,
            ringoa::TimeUnit::MICROSECONDS,
            /*show_details=*/true);
    }

    Logger::InfoLog(LOC, "OblivSelect ComputeDotProductBlockSIMD Benchmark completed");
    Logger::ExportLogListAndClear(kLogOsPath + "cdpb_simd_bench", /*use_timestamp=*/true);
}

void OblivSelect_EvaluateFullDomainThenDotProduct_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OblivSelect EvaluateFullDomainThenDotProduct Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters     params(db_bitsize);
        uint64_t                  n  = params.GetParameters().GetInputBitsize();
        uint64_t                  nu = params.GetParameters().GetTerminateBitsize();
        DpfKeyGenerator           gen(params.GetParameters());
        BinaryReplicatedSharing3P brss(n);
        OblivSelectEvaluator      eval_os(params, brss);

        uint64_t alpha = brss.GenerateRandomValue();
        uint64_t beta  = 1;

        std::vector<ringoa::block> uv_prev(1ULL << nu), uv_next(1ULL << nu);
        RepShareVec64              database_sh(1U << n);
        std::vector<uint64_t>      shuf_db_0(1U << n), shuf_db_1(1U << n);

        for (size_t i = 0; i < database_sh.Size(); ++i) {
            database_sh[0][i] = i;
            database_sh[1][i] = i;
        }

        uint64_t                  pr_prev   = brss.GenerateRandomValue();
        uint64_t                  pr_next   = brss.GenerateRandomValue();
        std::pair<DpfKey, DpfKey> keys_next = gen.GenerateKeys(alpha, beta);
        std::pair<DpfKey, DpfKey> keys_prev = gen.GenerateKeys(alpha, beta);

        // --- Timer setup ---
        const std::string timer_name = "OblivSelect EvaluateFullDomainThenDotProduct";
        TimerManager      timer_mgr;
        int32_t           timer_id = timer_mgr.CreateNewTimer(timer_name);
        timer_mgr.SelectTimer(timer_id);

        // --- Measurement loop ---
        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            eval_os.EvaluateFullDomainThenDotProduct(
                keys_prev.first, keys_next.second,
                uv_prev, uv_next,
                RepShareView64(database_sh), pr_prev, pr_next);
            timer_mgr.Stop(
                "n=" + ToString(n) +
                " nu=" + ToString(nu) +
                " algo=OS-EvalFullDomainThenDotProduct" +
                " iter=" + ToString(i));
        }

        // --- Summarize results ---
        const std::string summary_msg =
            "n=" + ToString(n) +
            " nu=" + ToString(nu) +
            " algo=OS-EvalFullDomainThenDotProduct";

        timer_mgr.PrintCurrentResults(
            summary_msg,
            ringoa::TimeUnit::MICROSECONDS,
            /*show_details=*/true);
    }

    Logger::InfoLog(LOC, "OblivSelect EvaluateFullDomainThenDotProduct Benchmark completed");
    Logger::ExportLogListAndClear(kLogOsPath + "efddp_bench", /*use_timestamp=*/true);
}

void OblivSelect_SingleBitMask_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OblivSelect SingleBitMask Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t                  d = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, bss);
        ShareIo                   sh_io;
        KeyIo                     key_io;
        TimerManager              timer_mgr;

        std::string key_path = kBenchOsPath + "oskeySBM_d" + ToString(d);
        std::string db_path  = kBenchOsPath + "db_bin_d" + ToString(d);
        std::string idx_path = kBenchOsPath + "idx_bin_d" + ToString(d);

        {    // KeyGen
            const std::string timer_name = "OblivSelect KeyGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                for (size_t p = 0; p < 3; ++p)
                    key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
            }
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // OfflineSetUp
            const std::string timer_name = "OblivSelect OfflineSetUp";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            brss.OfflineSetUp(kBenchOsPath + "prf");
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // DataGen
            const std::string timer_name = "OblivSelect DataGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            std::vector<block> database(1ULL << d);
            for (size_t i = 0; i < database.size(); ++i)
                database[i] = ringoa::MakeBlock(0, i);
            uint64_t index = bss.GenerateRandomValue();
            timer_mgr.Mark("DataGen d=" + ToString(d));
            std::array<RepShareVecBlock, 3> database_sh = brss.ShareLocal(database);
            std::array<RepShare64, 3>       index_sh    = brss.ShareLocal(index);
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

    Logger::InfoLog(LOC, "OblivSelect SingleBitMask Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogOsPath + "sbm_offline_bench", true);
}

void OblivSelect_SingleBitMask_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id    = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network     = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OblivSelect SingleBitMask Online Benchmark started (repeat=" + ToString(repeat) +
                             ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                OblivSelectParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t    d        = params.GetParameters().GetInputBitsize();
                std::string key_path = kBenchOsPath + "oskeySBM_d" + ToString(d);
                std::string db_path  = kBenchOsPath + "db_bin_d" + ToString(d);
                std::string idx_path = kBenchOsPath + "idx_bin_d" + ToString(d);

                TimerManager      timer_mgr;
                const std::string timer_setup_name = "OblivSelect (SBM) OnlineSetUp " + ptag;
                const std::string timer_eval_name  = "OblivSelect (SBM) Eval " + ptag;
                int32_t           timer_setup      = timer_mgr.CreateNewTimer(timer_setup_name);
                int32_t           timer_eval       = timer_mgr.CreateNewTimer(timer_eval_name);

                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(p, chl_prev, chl_next);
                RepShareBlock             result_sh;

                OblivSelectKey key(p, params);
                KeyIo          key_io;
                key_io.LoadKey(key_path + "_" + ToString(p), key);

                RepShareVecBlock database_sh;
                RepShare64       index_sh;
                ShareIo          sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(p), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(p), index_sh);

                brss.OnlineSetUp(p, kBenchOsPath + "prf");

                timer_mgr.Stop("d=" + ToString(d) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);

                timer_mgr.SelectTimer(timer_eval);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
                    timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                    if (i < 2)
                        Logger::InfoLog(LOC, "d=" + ToString(d) + " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
            }
        };
    };

    auto task0 = MakeTask(0);
    auto task1 = MakeTask(1);
    auto task2 = MakeTask(2);

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task0, task1, task2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OblivSelect SingleBitMask Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogOsPath + "sbm_online_p" + ToString(party_id) + "_" + network, true);
}

void OblivSelect_ShiftedAdditive_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OblivSelect ShiftedAdditive Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t                  d = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, bss);
        ShareIo                   sh_io;
        KeyIo                     key_io;
        TimerManager              timer_mgr;

        std::string key_path = kBenchOsPath + "oskeySA_d" + ToString(d);
        std::string db_path  = kBenchOsPath + "db_d" + ToString(d);
        std::string idx_path = kBenchOsPath + "idx_d" + ToString(d);

        {    // KeyGen
            const std::string timer_name = "OblivSelect KeyGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                for (size_t p = 0; p < 3; ++p)
                    key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
            }
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // OfflineSetUp
            const std::string timer_name = "OblivSelect OfflineSetUp";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            brss.OfflineSetUp(kBenchOsPath + "prf");
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");
            timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        }

        {    // DataGen
            const std::string timer_name = "OblivSelect DataGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);
            timer_mgr.Start();
            std::vector<uint64_t> database(1ULL << d);
            for (size_t i = 0; i < database.size(); ++i)
                database[i] = i;
            uint64_t index = bss.GenerateRandomValue();
            timer_mgr.Mark("DataGen d=" + ToString(d));
            std::array<RepShareVec64, 3> database_sh = brss.ShareLocal(database);
            std::array<RepShare64, 3>    index_sh    = brss.ShareLocal(index);
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

    Logger::InfoLog(LOC, "OblivSelect ShiftedAdditive Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogOsPath + "sa_offline_bench", true);
}

void OblivSelect_ShiftedAdditive_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat      = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id    = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network     = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> db_bitsizes = SelectBitsizes(cmd);

    Logger::InfoLog(LOC, "OblivSelect ShiftedAdditive Online Benchmark started (repeat=" + ToString(repeat) +
                             ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                OblivSelectParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t    d        = params.GetParameters().GetInputBitsize();
                uint64_t    nu       = params.GetParameters().GetTerminateBitsize();
                std::string key_path = kBenchOsPath + "oskeySA_d" + ToString(d);
                std::string db_path  = kBenchOsPath + "db_d" + ToString(d);
                std::string idx_path = kBenchOsPath + "idx_d" + ToString(d);

                TimerManager      timer_mgr;
                const std::string timer_setup_name = "OblivSelect (SA) OnlineSetUp " + ptag;
                const std::string timer_eval_name  = "OblivSelect (SA) Eval " + ptag;
                int32_t           timer_setup      = timer_mgr.CreateNewTimer(timer_setup_name);
                int32_t           timer_eval       = timer_mgr.CreateNewTimer(timer_eval_name);

                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(p, chl_prev, chl_next);
                RepShare64                result_sh;

                OblivSelectKey key(p, params);
                KeyIo          key_io;
                key_io.LoadKey(key_path + "_" + ToString(p), key);

                RepShareVec64 database_sh;
                RepShare64    index_sh;
                ShareIo       sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(p), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(p), index_sh);

                std::vector<ringoa::block> uv_prev(1ULL << nu), uv_next(1ULL << nu);
                brss.OnlineSetUp(p, kBenchOsPath + "prf");

                timer_mgr.Stop("d=" + ToString(d) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);

                timer_mgr.SelectTimer(timer_eval);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(chls, key, uv_prev, uv_next, RepShareView64(database_sh), index_sh, result_sh);
                    timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));
                    if (i < 2)
                        Logger::InfoLog(LOC, "d=" + ToString(d) + " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.PrintCurrentResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
            }
        };
    };

    auto task0 = MakeTask(0);
    auto task1 = MakeTask(1);
    auto task2 = MakeTask(2);

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task0, task1, task2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OblivSelect ShiftedAdditive Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogOsPath + "sa_online_p" + ToString(party_id) + "_" + network, true);
}

}    // namespace bench_ringoa
