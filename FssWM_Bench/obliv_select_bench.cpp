#include "obliv_select_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/protocol/key_io.h"
#include "FssWM/protocol/mixed_obliv_select.h"
#include "FssWM/protocol/obliv_select.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"

namespace {

const std::string kCurrentPath = fsswm::GetCurrentDirectory();
const std::string kBenchOSPath = kCurrentPath + "/data/bench/os/";

std::vector<uint64_t> db_bitsizes = {16, 18, 20, 22, 24, 26, 28};
// std::vector<uint64_t> db_bitsizes = fsswm::CreateSequence(10, 28);

constexpr uint64_t kRepeatDefault = 10;

}    // namespace

namespace bench_fsswm {

using fsswm::block;
using fsswm::Channels;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::ThreePartyNetworkManager;
using fsswm::TimerManager;
using fsswm::ToString, fsswm::Format;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::proto::KeyIo;
using fsswm::proto::MixedOblivSelectEvaluator;
using fsswm::proto::MixedOblivSelectKey;
using fsswm::proto::MixedOblivSelectKeyGenerator;
using fsswm::proto::MixedOblivSelectParameters;
using fsswm::proto::OblivSelectEvaluator;
using fsswm::proto::OblivSelectKey;
using fsswm::proto::OblivSelectKeyGenerator;
using fsswm::proto::OblivSelectParameters;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareBlock;
using fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::RepShareView64, fsswm::sharing::RepShareViewBlock;
using fsswm::sharing::ShareIo;

void OblivSelect_ComputeDotProductBlockSIMD_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_ComputeDotProductBlockSIMD_Bench...");
    uint64_t repeat = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;
    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters     params(db_bitsize);
        uint64_t                  n = params.GetParameters().GetInputBitsize();
        DpfKeyGenerator           gen(params.GetParameters());
        DpfEvaluator              eval(params.GetParameters());
        BinaryReplicatedSharing3P brss(n);
        OblivSelectEvaluator      eval_os(params, brss);
        uint64_t                  alpha = brss.GenerateRandomValue();
        uint64_t                  beta  = 1;
        RepShareVecBlock          database_sh(1U << n);
        for (size_t i = 0; i < database_sh.Size(); ++i) {
            database_sh[0][i] = fsswm::MakeBlock(0, i);
            database_sh[1][i] = fsswm::MakeBlock(0, i);
        }
        uint64_t pr_prev = brss.GenerateRandomValue();
        uint64_t pr_next = brss.GenerateRandomValue();

        TimerManager timer_mgr;
        // Generate keys
        std::pair<DpfKey, DpfKey> keys_next = gen.GenerateKeys(alpha, beta);
        std::pair<DpfKey, DpfKey> keys_prev = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        int32_t timer = timer_mgr.CreateNewTimer("OblivSelect:ComputeDotProductBlockSIMD");
        // Evaluate keys
        timer_mgr.SelectTimer(timer);

        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            block result_prev = eval_os.ComputeDotProductBlockSIMD(keys_prev.first, database_sh[0], pr_prev);
            block result_next = eval_os.ComputeDotProductBlockSIMD(keys_next.second, database_sh[1], pr_next);
            Logger::InfoLog(LOC, "Result Prev: " + Format(result_prev) + ", Result Next: " + Format(result_next));
            timer_mgr.Stop("n=" + ToString(db_bitsize) + " (" + ToString(i) + ")");
        }
        timer_mgr.PrintCurrentResults("n=" + ToString(db_bitsize), fsswm::TimeUnit::MICROSECONDS, true);
    }
    Logger::InfoLog(LOC, "OblivSelect_ComputeDotProductBlockSIMD_Bench - Finished");
}

void OblivSelect_EvaluateFullDomainThenDotProduct_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_EvaluateFullDomainThenDotProduct_Bench...");
    uint64_t repeat = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;
    for (auto db_bitsize : db_bitsizes) {
        OblivSelectParameters     params(db_bitsize);
        uint64_t                  n  = params.GetParameters().GetInputBitsize();
        uint64_t                  nu = params.GetParameters().GetTerminateBitsize();
        DpfKeyGenerator           gen(params.GetParameters());
        DpfEvaluator              eval(params.GetParameters());
        BinaryReplicatedSharing3P brss(n);
        OblivSelectEvaluator      eval_os(params, brss);
        uint64_t                  alpha = brss.GenerateRandomValue();
        uint64_t                  beta  = 1;

        std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
        RepShareVec64             database_sh(1U << n);
        std::vector<uint64_t>     shuf_db_0(1U << n), shuf_db_1(1U << n);
        for (size_t i = 0; i < database_sh.Size(); ++i) {
            database_sh[0][i] = i;
            database_sh[1][i] = i;
        }
        uint64_t                  pr_prev   = brss.GenerateRandomValue();
        uint64_t                  pr_next   = brss.GenerateRandomValue();
        std::pair<DpfKey, DpfKey> keys_next = gen.GenerateKeys(alpha, beta);
        std::pair<DpfKey, DpfKey> keys_prev = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        TimerManager timer_mgr;
        int32_t      timer = timer_mgr.CreateNewTimer("OblivSelect:EvaluateFullDomainThenDotProduct");
        // Evaluate keys
        timer_mgr.SelectTimer(timer);

        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            eval_os.EvaluateFullDomainThenDotProduct(
                keys_prev.first, keys_next.second,
                uv_prev, uv_next,
                RepShareView64(database_sh), pr_prev, pr_next);
            timer_mgr.Stop("n=" + ToString(db_bitsize) + " (" + ToString(i) + ")");
        }
        timer_mgr.PrintCurrentResults("n=" + ToString(db_bitsize), fsswm::TimeUnit::MICROSECONDS, true);
    }
    Logger::InfoLog(LOC, "OblivSelect_EvaluateFullDomainThenDotProduct_Bench - Finished");
}

void OblivSelect_Binary_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_Binary_Offline_Bench...");
    uint64_t repeat = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;

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

        std::string key_path = kBenchOSPath + "oskeySBM_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "dbSBM_d" + ToString(d);
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
    Logger::InfoLog(LOC, "OblivSelect_Binary_Offline_Bench - Finished");
}

void OblivSelect_Binary_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_Binary_Online_Bench...");
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    uint64_t    repeat   = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Helper that returns a task lambda for a given party_id
    auto MakeTask = [&](int party_id) {
        // Capture d, params, paths, sh_io, key_io, repeat
        return [=](osuCrypto::Channel &chl_next,
                   osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                OblivSelectParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t d = params.GetParameters().GetInputBitsize();
                FileIo   file_io;

                std::string key_path = kBenchOSPath + "oskeySBM_d" + ToString(d);
                std::string db_path  = kBenchOSPath + "dbSBM_d" + ToString(d);
                std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);
                // (1) Set up TimerManager and timers
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

                // (2) Begin SetUp timing
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                // (3) Set up the binary replicated-sharing object and evaluator
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(party_id, chl_prev, chl_next);
                RepShareBlock             result_sh;

                // (4) Load this party’s key
                OblivSelectKey key(party_id, params);
                KeyIo          key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (5) Load this party’s database share and index share
                RepShareVecBlock database_sh;
                RepShare64       index_sh;
                ShareIo          sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                // (6) Setup PRF keys
                brss.OnlineSetUp(party_id, kBenchOSPath + "prf");

                // (7) Stop SetUp timer
                timer_mgr.Stop("SetUp d=" + ToString(d));

                // (8) Begin Eval timing
                timer_mgr.SelectTimer(timer_eval);

                // (9) Repeat Evaluate and measure each iteration
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(chls,
                                  key,
                                  RepShareViewBlock(database_sh),
                                  index_sh,
                                  result_sh);
                    timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));

                    Logger::InfoLog(LOC, "Total data sent: " +
                                             ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }

                // (10) Print all timing results
                timer_mgr.PrintAllResults("d=" + ToString(d), fsswm::MICROSECONDS, true);
            }
        };
    };

    // Create tasks for parties 0, 1, and 2
    auto task0 = MakeTask(0);
    auto task1 = MakeTask(1);
    auto task2 = MakeTask(2);

    // Configure network based on party ID (from CLI/env) and wait for completion
    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task0, task1, task2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OblivSelect_Binary_Online_Bench - Finished");
}

void OblivSelect_Additive_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_Additive_Offline_Bench...");
    uint64_t repeat = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;

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

        std::string key_path = kBenchOSPath + "oskeySA_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "dbSA_d" + ToString(d);
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
        std::vector<uint64_t> database(1U << d);
        for (size_t i = 0; i < database.size(); ++i) {
            database[i] = i;
        }
        uint64_t index = bss.GenerateRandomValue();
        timer_mgr.Mark("DataGen d=" + ToString(d));

        std::array<RepShareVec64, 3> database_sh = brss.ShareLocal(database);
        std::array<RepShare64, 3>    index_sh    = brss.ShareLocal(index);
        timer_mgr.Mark("ShareGen d=" + ToString(d));

        // Save data
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }
        timer_mgr.Mark("ShareSave d=" + ToString(d));
        timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), fsswm::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "OblivSelect_Additive_Offline_Bench - Finished");
}

void OblivSelect_Additive_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "OblivSelect_Additive_Online_Bench...");
    uint64_t    repeat   = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Helper that returns a task lambda for a given party_id
    auto MakeTask = [&](int party_id) {
        // Capture d, nu, params, and paths
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                OblivSelectParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t d  = params.GetParameters().GetInputBitsize();
                uint64_t nu = params.GetParameters().GetTerminateBitsize();

                std::string key_path = kBenchOSPath + "oskeySA_d" + ToString(d);
                std::string db_path  = kBenchOSPath + "dbSA_d" + ToString(d);
                std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

                // (1) Set up TimerManager and timers
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

                // (2) Begin SetUp timing
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                // (3) Set up the replicated-sharing object and evaluator
                BinaryReplicatedSharing3P brss(d);
                OblivSelectEvaluator      eval(params, brss);
                Channels                  chls(party_id, chl_prev, chl_next);
                RepShare64                result_sh;

                // (4) Load this party’s key
                OblivSelectKey key(party_id, params);
                KeyIo          key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (5) Load this party’s shares of both databases and the index
                RepShareVec64             database_sh, _sh;
                RepShare64                index_sh;
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
                ShareIo                   sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                // (6) Setup PRF keys
                brss.OnlineSetUp(party_id, kBenchOSPath + "prf");

                // (7) Stop SetUp timer
                timer_mgr.Stop("SetUp d=" + ToString(d));

                // (8) Begin Eval timing
                timer_mgr.SelectTimer(timer_eval);

                // (9) Repeat Evaluate and measure each iteration
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(
                        chls,
                        key,
                        uv_prev, uv_next,
                        RepShareView64(database_sh),
                        index_sh,
                        result_sh);
                    timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));

                    if (i < 2)
                        Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }

                // (10) Print all timing results
                timer_mgr.PrintAllResults("d=" + ToString(d), fsswm::MICROSECONDS, true);
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

    Logger::InfoLog(LOC, "OblivSelect_Additive_Online_Bench - Finished");
}

void MixedOblivSelect_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "MixedOblivSelect_Offline_Bench...");
    uint64_t repeat = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;

    for (auto db_bitsize : db_bitsizes) {
        MixedOblivSelectParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t                     d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P            ass(d);
        ReplicatedSharing3P          rss(d);
        MixedOblivSelectKeyGenerator gen(params, ass);
        FileIo                       file_io;
        ShareIo                      sh_io;
        KeyIo                        key_io;

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("MixedOblivSelect KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("MixedOblivSelect OfflineSetUp");

        std::string key_path = kBenchOSPath + "mixoskey_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "mixdb_d" + ToString(d);
        std::string idx_path = kBenchOSPath + "mixidx_d" + ToString(d);

        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::array<MixedOblivSelectKey, 3> keys = gen.GenerateKeys();
            // Save keys
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);
            timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d));

            timer_mgr.SelectTimer(timer_off);
            timer_mgr.Start();
            // Offline setup
            gen.OfflineSetUp(repeat, kBenchOSPath);
            rss.OfflineSetUp(kBenchOSPath + "prf");
            timer_mgr.Stop("OfflineSetUp(" + ToString(i) + ") d=" + ToString(d));
        }
        timer_mgr.PrintAllResults("Gen d=" + ToString(d), fsswm::MICROSECONDS, true);

        // Generate the database and index
        int32_t timer_data = timer_mgr.CreateNewTimer("OS DataGen");
        timer_mgr.SelectTimer(timer_data);
        timer_mgr.Start();
        std::vector<uint64_t> database(1U << d);
        for (size_t i = 0; i < database.size(); ++i) {
            database[i] = i;
        }
        uint64_t index = ass.GenerateRandomValue();
        timer_mgr.Mark("DataGen d=" + ToString(d));

        std::array<RepShareVec64, 3> database_sh = rss.ShareLocal(database);
        std::array<RepShare64, 3>    index_sh    = rss.ShareLocal(index);
        timer_mgr.Mark("ShareGen d=" + ToString(d));

        // Save data
        for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
            sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        }
        timer_mgr.Mark("ShareSave d=" + ToString(d));
        timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), fsswm::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "MixedOblivSelect_Offline_Bench - Finished");
}

void MixedOblivSelect_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "MixedOblivSelect_Additive_Online_Bench...");
    uint64_t    repeat   = cmd.isSet("repeat") ? cmd.get<uint64_t>("repeat") : kRepeatDefault;
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Helper that returns a task lambda for a given party_id
    auto MakeTask = [&](int party_id) {
        // Capture d, nu, params, and paths
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                MixedOblivSelectParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t d  = params.GetParameters().GetInputBitsize();
                uint64_t nu = params.GetParameters().GetTerminateBitsize();

                std::string key_path = kBenchOSPath + "mixoskey_d" + ToString(d);
                std::string db_path  = kBenchOSPath + "mixdb_d" + ToString(d);
                std::string idx_path = kBenchOSPath + "mixidx_d" + ToString(d);

                // (1) Set up TimerManager and timers
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("OS SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("OS Eval");

                // (2) Begin SetUp timing
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                // (3) Set up the replicated-sharing object and evaluator
                ReplicatedSharing3P       rss(d);
                AdditiveSharing2P         ass_prev(d), ass_next(d);
                MixedOblivSelectEvaluator eval(params, rss, ass_prev, ass_next);
                Channels                  chls(party_id, chl_prev, chl_next);
                RepShare64                result_sh;

                // (4) Load this party’s key
                MixedOblivSelectKey key(party_id, params);
                KeyIo               key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (5) Load this party’s shares of both databases and the index
                RepShareVec64             database_sh, _sh;
                RepShare64                index_sh;
                std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
                ShareIo                   sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                // (6) Setup PRF keys
                eval.OnlineSetUp(party_id, kBenchOSPath);
                rss.OnlineSetUp(party_id, kBenchOSPath + "prf");

                // (7) Stop SetUp timer
                timer_mgr.Stop("SetUp d=" + ToString(d));

                // (8) Begin Eval timing
                timer_mgr.SelectTimer(timer_eval);

                // (9) Repeat Evaluate and measure each iteration
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    eval.Evaluate(
                        chls,
                        key,
                        uv_prev, uv_next,
                        RepShareView64(database_sh),
                        index_sh,
                        result_sh);
                    timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d));

                    if (i < 2)
                        Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + " bytes");
                    chls.ResetStats();
                }

                // (10) Print all timing results
                timer_mgr.PrintAllResults("d=" + ToString(d), fsswm::MICROSECONDS, true);
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

    Logger::InfoLog(LOC, "MixedOblivSelect_Additive_Online_Bench - Finished");
}

}    // namespace bench_fsswm
