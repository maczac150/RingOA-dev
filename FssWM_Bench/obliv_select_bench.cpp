#include "obliv_select_bench.h"

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/key_io.h"
#include "FssWM/wm/obliv_select.h"

namespace {

const std::string kCurrentPath = fsswm::GetCurrentDirectory();
const std::string kBenchOSPath = kCurrentPath + "/data/bench/os/";

// std::vector<uint32_t> db_bitsizes     = {12, 14, 16, 18, 20, 22, 24, 26};
std::vector<uint32_t> db_bitsizes     = {22, 22, 22, 22, 22};
std::vector<uint32_t> db_bitsizes_all = fsswm::CreateSequence(10, 27);

constexpr uint32_t repeat = 30;

}    // namespace

namespace bench_fsswm {

using fsswm::block;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::ShareType;
using fsswm::ThreePartyNetworkManager;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::Channels;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::RepShare;
using fsswm::sharing::RepShareVec;
using fsswm::sharing::ShareIo;
using fsswm::wm::KeyIo;
using fsswm::wm::OblivSelectEvaluator;
using fsswm::wm::OblivSelectKey;
using fsswm::wm::OblivSelectKeyGenerator;
using fsswm::wm::OblivSelectParameters;

void OblivSelect_Offline_Bench() {
    Logger::InfoLog(LOC, "OblivSelect_Offline_Bench...");

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters params(db_bitsize, ShareType::kBinary);
        params.PrintParameters();
        uint32_t                  d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P         ass(d);
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, ass, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("OblivSelect KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("OblivSelect OfflineSetUp");

        std::string key_path = kBenchOSPath + "oskey_d" + std::to_string(d);
        std::string db_path  = kBenchOSPath + "db_d" + std::to_string(d);
        std::string idx_path = kBenchOSPath + "idx_d" + std::to_string(d);

        for (uint32_t i = 0; i < repeat; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();
            // Save keys
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);
            timer_mgr.Stop("KeyGen(" + std::to_string(i) + ") d=" + std::to_string(d));

            timer_mgr.SelectTimer(timer_off);
            timer_mgr.Start();
            // Offline setup
            brss.OfflineSetUp(kBenchOSPath + "prf");
            timer_mgr.Stop("OfflineSetUp(" + std::to_string(i) + ") d=" + std::to_string(d));
        }
        timer_mgr.PrintAllResults("Gen d=" + std::to_string(d), fsswm::MICROSECONDS, true);

        // // Generate the database and index
        // int32_t timer_data = timer_mgr.CreateNewTimer("OS DataGen");
        // timer_mgr.SelectTimer(timer_data);
        // timer_mgr.Start();
        // std::vector<uint32_t> database = fsswm::CreateSequence(0, 1U << d);
        // uint32_t              index    = ass.GenerateRandomValue();
        // timer_mgr.Mark("DataGen d=" + std::to_string(d));

        // std::array<RepShareVec, 3> database_sh = brss.ShareLocal(database);
        // std::array<RepShare, 3>    index_sh    = brss.ShareLocal(index);
        // timer_mgr.Mark("ShareGen d=" + std::to_string(d));

        // // Save data
        // for (size_t p = 0; p < fsswm::sharing::kNumParties; ++p) {
        //     sh_io.SaveShare(db_path + "_" + std::to_string(p), database_sh[p]);
        //     sh_io.SaveShare(idx_path + "_" + std::to_string(p), index_sh[p]);
        // }
        // timer_mgr.Mark("ShareSave d=" + std::to_string(d));
        // timer_mgr.PrintCurrentResults("DataGen d=" + std::to_string(d), fsswm::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "OblivSelect_Offline_Bench - Finished");
    // FileIo io(".log");
    // io.WriteToFile("./data/log/os_offline", Logger::GetLogList());
}

void OblivSelect_Online_Bench(const oc::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_Online_Bench...");
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters params(db_bitsize, ShareType::kBinary);
        params.PrintParameters();
        uint32_t d  = params.GetParameters().GetInputBitsize();
        uint32_t nu = params.GetParameters().GetTerminateBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        std::string key_path = kBenchOSPath + "oskey_d" + std::to_string(d);
        std::string db_path  = kBenchOSPath + "db_d" + std::to_string(d);
        std::string idx_path = kBenchOSPath + "idx_d" + std::to_string(d);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

            for (uint32_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();
                // Setup
                ReplicatedSharing3P       rss(d);
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, rss, brss);
                Channels                  chls(0, chl_prev, chl_next);
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
                RepShare                  result_sh;

                // Load keys
                OblivSelectKey key(0, params);
                key_io.LoadKey(key_path + "_0", key);
                // Load data
                RepShareVec database_sh;
                RepShare    index_sh;
                sh_io.LoadShare(db_path + "_0", database_sh);
                sh_io.LoadShare(idx_path + "_0", index_sh);
                // Setup the PRF keys
                brss.OnlineSetUp(0, kBenchOSPath + "prf");
                timer_mgr.Stop("SetUp(" + std::to_string(i) + ") d=" + std::to_string(d));

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.Start();
                // Evaluate
                eval.EvaluateBinary(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
                timer_mgr.Stop("Eval(" + std::to_string(i) + ") d=" + std::to_string(d));
                Logger::InfoLog(LOC, "Total data sent: " + std::to_string(chls.GetStats()) + "bytes");
                chls.ResetStats();
            }
            timer_mgr.PrintAllResults("d=" + std::to_string(d), fsswm::MICROSECONDS, true);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

            for (uint32_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();
                // Setup
                ReplicatedSharing3P       rss(d);
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, rss, brss);
                Channels                  chls(1, chl_prev, chl_next);
                RepShare                  result_sh;
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);

                // Load keys
                OblivSelectKey key(1, params);
                key_io.LoadKey(key_path + "_1", key);
                // Load data
                RepShareVec database_sh;
                RepShare    index_sh;
                sh_io.LoadShare(db_path + "_1", database_sh);
                sh_io.LoadShare(idx_path + "_1", index_sh);
                // Setup the PRF keys
                brss.OnlineSetUp(1, kBenchOSPath + "prf");
                timer_mgr.Stop("SetUp(" + std::to_string(i) + ") d=" + std::to_string(d));

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.Start();
                // Evaluate
                eval.EvaluateBinary(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
                timer_mgr.Stop("Eval(" + std::to_string(i) + ") d=" + std::to_string(d));
                Logger::InfoLog(LOC, "Total data sent: " + std::to_string(chls.GetStats()) + "bytes");
                chls.ResetStats();
            }
            timer_mgr.PrintAllResults("d=" + std::to_string(d), fsswm::MICROSECONDS, true);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

            for (uint32_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();
                // Setup
                ReplicatedSharing3P       rss(d);
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, rss, brss);
                Channels                  chls(2, chl_prev, chl_next);
                RepShare                  result_sh;
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);

                // Load keys
                OblivSelectKey key(2, params);
                key_io.LoadKey(key_path + "_2", key);
                // Load data
                RepShareVec database_sh;
                RepShare    index_sh;
                sh_io.LoadShare(db_path + "_2", database_sh);
                sh_io.LoadShare(idx_path + "_2", index_sh);
                // Setup the PRF keys
                brss.OnlineSetUp(2, kBenchOSPath + "prf");
                timer_mgr.Stop("SetUp(" + std::to_string(i) + ") d=" + std::to_string(d));

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.Start();
                // Evaluate
                eval.EvaluateBinary(chls, uv_prev, uv_next, key, database_sh, index_sh, result_sh);
                timer_mgr.Stop("Eval(" + std::to_string(i) + ") d=" + std::to_string(d));
                Logger::InfoLog(LOC, "Total data sent: " + std::to_string(chls.GetStats()) + "bytes");
                chls.ResetStats();
            }
            timer_mgr.PrintAllResults("d=" + std::to_string(d), fsswm::MICROSECONDS, true);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();
    }
    Logger::InfoLog(LOC, "OblivSelect_Online_Bench - Finished");
}

void OblivSelect_DotProduct_Bench() {
    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters     params(db_bitsize, ShareType::kBinary);
        uint32_t                  n  = params.GetParameters().GetInputBitsize();
        uint32_t                  e  = params.GetParameters().GetOutputBitsize();
        uint32_t                  nu = params.GetParameters().GetTerminateBitsize();
        DpfKeyGenerator           gen(params.GetParameters());
        DpfEvaluator              eval(params.GetParameters());
        ReplicatedSharing3P       rss(n);
        BinaryReplicatedSharing3P brss(n);
        OblivSelectEvaluator      eval_os(params, rss, brss);
        uint32_t                  alpha = 10;
        uint32_t                  beta  = 1;
        RepShareVec               database_sh;
        database_sh[0].resize(1U << n, 1);
        database_sh[1].resize(1U << n, 1);

        TimerManager timer_mgr;
        int32_t      timer_1 = timer_mgr.CreateNewTimer("OS DotProduct1");

        // Generate keys
        std::pair<DpfKey, DpfKey> keys_next = gen.GenerateKeys(alpha, beta);
        std::pair<DpfKey, DpfKey> keys_prev = gen.GenerateKeys(alpha, beta);
        std::vector<block>        uv_prev(1U << nu), uv_next(1U << nu);

        // Evaluate keys
        timer_mgr.SelectTimer(timer_1);
        for (uint32_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            eval.EvaluateFullDomain(keys_prev.first, uv_prev);
            eval.EvaluateFullDomain(keys_next.second, uv_next);
            auto [dp_prev, dp_next] = eval_os.BinaryDotProduct(uv_prev, uv_next, 1, 1, database_sh);
            timer_mgr.Stop("n=" + std::to_string(db_bitsize) + " (" + std::to_string(i) + ")");
        }
        timer_mgr.PrintCurrentResults("n=" + std::to_string(db_bitsize), fsswm::TimeUnit::MICROSECONDS, true);

        int32_t timer_2 = timer_mgr.CreateNewTimer("OS DotProduct2");
        // Evaluate keys
        timer_mgr.SelectTimer(timer_2);
        for (uint32_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            uint32_t dp_prev = eval_os.FullDomainDotProduct(keys_prev.first, database_sh[0], 1);
            uint32_t dp_next = eval_os.FullDomainDotProduct(keys_next.second, database_sh[1], 1);
            timer_mgr.Stop("n=" + std::to_string(db_bitsize) + " (" + std::to_string(i) + ")");
        }
        timer_mgr.PrintCurrentResults("n=" + std::to_string(db_bitsize), fsswm::TimeUnit::MICROSECONDS, true);
    }
}

}    // namespace bench_fsswm
