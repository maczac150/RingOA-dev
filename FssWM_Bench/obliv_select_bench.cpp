#include "obliv_select_bench.h"

#include <cryptoTools/Common/TestCollection.h>

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

std::vector<uint64_t> db_bitsizes     = {12, 14, 16, 18, 20, 22, 24, 26};
std::vector<uint64_t> db_bitsizes_all = fsswm::CreateSequence(10, 27);

constexpr uint64_t repeat = 30;

}    // namespace

namespace bench_fsswm {

using fsswm::block;
using fsswm::Channels;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::ThreePartyNetworkManager;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareBlock;
using fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::RepShareViewBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::KeyIo;
using fsswm::wm::OblivSelectEvaluator;
using fsswm::wm::OblivSelectKey;
using fsswm::wm::OblivSelectKeyGenerator;
using fsswm::wm::OblivSelectParameters;

void OblivSelect_Offline_Bench() {
    Logger::InfoLog(LOC, "OblivSelect_Offline_Bench...");

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t                  d = params.GetParameters().GetInputBitsize();
        BinarySharing2P           bss(d);
        BinaryReplicatedSharing3P brss(d);
        OblivSelectKeyGenerator   gen(params, bss);
        FileIo                    file_io;
        ShareIo                   sh_io;
        KeyIo                     key_io;

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("OblivSelect KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("OblivSelect OfflineSetUp");

        std::string key_path = kBenchOSPath + "oskey_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "db_d" + ToString(d);
        std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::array<OblivSelectKey, 3> keys = gen.GenerateKeys();
            // Save keys
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);
            timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d));

            timer_mgr.SelectTimer(timer_off);
            timer_mgr.Start();
            // Offline setup
            brss.OfflineSetUp(kBenchOSPath + "prf");
            timer_mgr.Stop("OfflineSetUp(" + ToString(i) + ") d=" + ToString(d));
        }
        timer_mgr.PrintAllResults("Gen d=" + ToString(d), fsswm::MICROSECONDS, true);

        // Generate the database and index
        int32_t timer_data = timer_mgr.CreateNewTimer("OS DataGen");
        timer_mgr.SelectTimer(timer_data);
        timer_mgr.Start();
        std::vector<block> database(1U << d);
        for (size_t i = 0; i < database.size(); ++i) {
            database[i] = fsswm::MakeBlock(0, i);
        }
        uint64_t index = bss.GenerateRandomValue();
        timer_mgr.Mark("DataGen d=" + ToString(d));

        std::array<RepShareVecBlock, 3> database_sh = brss.ShareLocal(database);
        std::array<RepShare64, 3>       index_sh    = brss.ShareLocal(index);
        timer_mgr.Mark("ShareGen d=" + ToString(d));

        // Save data
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }
        timer_mgr.Mark("ShareSave d=" + ToString(d));
        timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), fsswm::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "OblivSelect_Offline_Bench - Finished");
    // FileIo io(".log");
    // io.WriteToFile("./data/log/os_offline", Logger::GetLogList());
}

void OblivSelect_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_Online_Bench...");
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t d  = params.GetParameters().GetInputBitsize();
        uint64_t nu = params.GetParameters().GetTerminateBitsize();
        FileIo   file_io;
        ShareIo  sh_io;
        KeyIo    key_io;

        std::string key_path = kBenchOSPath + "oskey_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "db_d" + ToString(d);
        std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

        // Define the task for each party
        ThreePartyNetworkManager net_mgr;

        // Party 0 task
        auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();
                // Setup
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(0, chl_prev, chl_next);
                RepShareBlock             result_sh;

                // Load keys
                OblivSelectKey key(0, params);
                key_io.LoadKey(key_path + "_0", key);
                // Load data
                RepShareVecBlock database_sh;
                RepShare64       index_sh;
                sh_io.LoadShare(db_path + "_0", database_sh);
                sh_io.LoadShare(idx_path + "_0", index_sh);
                // Setup the PRF keys
                brss.OnlineSetUp(0, kBenchOSPath + "prf");
                timer_mgr.Stop("SetUp(" + ToString(i) + ") d=" + ToString(d));

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.Start();
                // Evaluate
                eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
                eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
                timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));
                Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + "bytes");
                chls.ResetStats();
            }
            timer_mgr.PrintAllResults("d=" + ToString(d), fsswm::MICROSECONDS, true);
        };

        // Party 1 task
        auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();
                // Setup
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(1, chl_prev, chl_next);
                RepShareBlock             result_sh;

                // Load keys
                OblivSelectKey key(1, params);
                key_io.LoadKey(key_path + "_1", key);
                // Load data
                RepShareVecBlock database_sh;
                RepShare64       index_sh;
                sh_io.LoadShare(db_path + "_1", database_sh);
                sh_io.LoadShare(idx_path + "_1", index_sh);
                // Setup the PRF keys
                brss.OnlineSetUp(1, kBenchOSPath + "prf");
                timer_mgr.Stop("SetUp(" + ToString(i) + ") d=" + ToString(d));

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.Start();
                // Evaluate
                eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
                timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));
                Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + "bytes");
                chls.ResetStats();
            }
            timer_mgr.PrintAllResults("d=" + ToString(d), fsswm::MICROSECONDS, true);
        };

        // Party 2 task
        auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();
                // Setup
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(2, chl_prev, chl_next);
                RepShareBlock             result_sh;

                // Load keys
                OblivSelectKey key(2, params);
                key_io.LoadKey(key_path + "_2", key);
                // Load data
                RepShareVecBlock database_sh;
                RepShare64       index_sh;
                sh_io.LoadShare(db_path + "_2", database_sh);
                sh_io.LoadShare(idx_path + "_2", index_sh);
                // Setup the PRF keys
                brss.OnlineSetUp(2, kBenchOSPath + "prf");
                timer_mgr.Stop("SetUp(" + ToString(i) + ") d=" + ToString(d));

                timer_mgr.SelectTimer(timer_eval);
                timer_mgr.Start();
                // Evaluate
                eval.Evaluate(chls, key, RepShareViewBlock(database_sh), index_sh, result_sh);
                timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));
                Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + "bytes");
                chls.ResetStats();
            }
            timer_mgr.PrintAllResults("d=" + ToString(d), fsswm::MICROSECONDS, true);
        };

        // Configure network based on party ID and wait for completion
        net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
        net_mgr.WaitForCompletion();
    }
    Logger::InfoLog(LOC, "OblivSelect_Online_Bench - Finished");
}

void OblivSelect_DotProduct_Bench() {
    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters     params(db_bitsize);
        uint64_t                  n  = params.GetParameters().GetInputBitsize();
        uint64_t                  e  = params.GetParameters().GetOutputBitsize();
        uint64_t                  nu = params.GetParameters().GetTerminateBitsize();
        DpfKeyGenerator           gen(params.GetParameters());
        DpfEvaluator              eval(params.GetParameters());
        BinaryReplicatedSharing3P brss(n);
        OblivSelectEvaluator      eval_os(params, brss);
        uint64_t                  alpha = 10;
        uint64_t                  beta  = 1;
        RepShareVecBlock          database_sh(1U << n);

        TimerManager timer_mgr;
        // Generate keys
        std::pair<DpfKey, DpfKey> keys_next = gen.GenerateKeys(alpha, beta);
        std::pair<DpfKey, DpfKey> keys_prev = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        int32_t timer = timer_mgr.CreateNewTimer("OS DotProduct");
        // Evaluate keys
        timer_mgr.SelectTimer(timer);

        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            block dp_prev = eval_os.FullDomainDotProduct(keys_prev.first, database_sh[0], 1);
            block dp_next = eval_os.FullDomainDotProduct(keys_next.second, database_sh[1], 1);
            timer_mgr.Stop("n=" + ToString(db_bitsize) + " (" + ToString(i) + ")");
        }
        timer_mgr.PrintCurrentResults("n=" + ToString(db_bitsize), fsswm::TimeUnit::MICROSECONDS, true);
    }
}

}    // namespace bench_fsswm
