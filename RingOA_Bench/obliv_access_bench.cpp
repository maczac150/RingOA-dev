#include "obliv_access_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/key_io.h"
#include "RingOA/protocol/ringoa.h"
#include "RingOA/protocol/shared_ot.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/sharing/additive_3p.h"
#include "RingOA/sharing/binary_2p.h"
#include "RingOA/sharing/binary_3p.h"
#include "RingOA/sharing/share_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/utils.h"

namespace {

const std::string kCurrentPath = ringoa::GetCurrentDirectory();
const std::string kBenchOSPath = kCurrentPath + "/data/bench/os/";

// std::vector<uint64_t> db_bitsizes = {16, 18, 20, 22, 24, 26, 28};
std::vector<uint64_t> db_bitsizes = ringoa::CreateSequence(10, 31);

constexpr uint64_t kIterDefault = 10;

constexpr ringoa::fss::EvalType kEvalType = ringoa::fss::EvalType::kIterDepthFirst;

}    // namespace

namespace bench_ringoa {

using ringoa::block;
using ringoa::Channels;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::Mod;
using ringoa::ThreePartyNetworkManager;
using ringoa::TimerManager;
using ringoa::ToString, ringoa::Format;
using ringoa::fss::dpf::DpfEvaluator;
using ringoa::fss::dpf::DpfKey;
using ringoa::fss::dpf::DpfKeyGenerator;
using ringoa::fss::dpf::DpfParameters;
using ringoa::proto::KeyIo;
using ringoa::proto::RingOaEvaluator;
using ringoa::proto::RingOaKey;
using ringoa::proto::RingOaKeyGenerator;
using ringoa::proto::RingOaParameters;
using ringoa::proto::SharedOtEvaluator;
using ringoa::proto::SharedOtKey;
using ringoa::proto::SharedOtKeyGenerator;
using ringoa::proto::SharedOtParameters;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::BinaryReplicatedSharing3P;
using ringoa::sharing::BinarySharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64, ringoa::sharing::RepShareBlock;
using ringoa::sharing::RepShareVec64, ringoa::sharing::RepShareVecBlock;
using ringoa::sharing::RepShareView64, ringoa::sharing::RepShareViewBlock;
using ringoa::sharing::ShareIo;

void SharedOt_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "SharedOt_Offline_Bench...");
    uint64_t iter = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;

    for (auto db_bitsize : db_bitsizes) {
        SharedOtParameters params(db_bitsize, kEvalType);
        params.PrintParameters();
        uint64_t             d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P    ass(d);
        ReplicatedSharing3P  rss(d);
        SharedOtKeyGenerator gen(params, ass);
        FileIo               file_io;
        ShareIo              sh_io;
        KeyIo                key_io;

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("SharedOt KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("SharedOt OfflineSetUp");

        std::string key_path = kBenchOSPath + "sharedotkey_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "db_d" + ToString(d);
        std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

        for (uint64_t i = 0; i < iter; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::array<SharedOtKey, 3> keys = gen.GenerateKeys();
            timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d));
            // Save keys
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);
        }

        timer_mgr.SelectTimer(timer_off);
        timer_mgr.Start();
        // Offline setup
        rss.OfflineSetUp(kBenchOSPath + "prf");
        timer_mgr.Stop("OfflineSetUp(0) d=" + ToString(d));
        timer_mgr.PrintAllResults("Gen d=" + ToString(d), ringoa::MICROSECONDS, true);

        // // Generate the database and index
        // int32_t timer_data = timer_mgr.CreateNewTimer("OS DataGen");
        // timer_mgr.SelectTimer(timer_data);
        // timer_mgr.Start();
        // std::vector<uint64_t> database(1U << d);
        // for (size_t i = 0; i < database.size(); ++i) {
        //     database[i] = i;
        // }
        // uint64_t index = ass.GenerateRandomValue();
        // timer_mgr.Mark("DataGen d=" + ToString(d));

        // std::array<RepShareVec64, 3> database_sh = rss.ShareLocal(database);
        // std::array<RepShare64, 3>    index_sh    = rss.ShareLocal(index);
        // timer_mgr.Mark("ShareGen d=" + ToString(d));

        // // Save data
        // for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
        //     sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
        //     sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        // }
        // timer_mgr.Mark("ShareSave d=" + ToString(d));
        // timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), ringoa::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "SharedOt_Offline_Bench - Finished");
    if (kEvalType == ringoa::fss::EvalType::kIterSingleBatch) {
        Logger::ExportLogList("./data/logs/oa/sharedot_offline_bench");
    } else {
        Logger::ExportLogList("./data/logs/oa/sharedot_naive_offline_bench");
    }
}

void SharedOt_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "SharedOt_Online_Bench...");
    uint64_t    iter     = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Helper that returns a task lambda for a given party_id
    auto MakeTask = [&](int party_id) {
        // Capture d, nu, params, and paths
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                SharedOtParameters params(db_bitsize, kEvalType);
                params.PrintParameters();
                uint64_t d = params.GetParameters().GetInputBitsize();

                std::string key_path = kBenchOSPath + "sharedotkey_d" + ToString(d);
                std::string db_path  = kBenchOSPath + "db_d" + ToString(d);
                std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

                // (1) Set up TimerManager and timers
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("SharedOT SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("SharedOT Eval");

                // (2) Begin SetUp timing
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                // (3) Set up the replicated-sharing object and evaluator
                ReplicatedSharing3P rss(d);
                SharedOtEvaluator   eval(params, rss);
                Channels            chls(party_id, chl_prev, chl_next);
                RepShare64          result_sh;

                // (4) Load this party’s key
                SharedOtKey key(party_id, params);
                KeyIo       key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (5) Load this party’s shares of both databases and the index
                RepShareVec64         database_sh, _sh;
                RepShare64            index_sh;
                std::vector<uint64_t> uv_prev(1U << d), uv_next(1U << d);
                ShareIo               sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                // (6) Setup PRF keys
                rss.OnlineSetUp(party_id, kBenchOSPath + "prf");

                // (7) Stop SetUp timer
                timer_mgr.Stop("SetUp d=" + ToString(d));

                // (8) Begin Eval timing
                timer_mgr.SelectTimer(timer_eval);

                // (9) iter Evaluate and measure each iteration
                for (uint64_t i = 0; i < iter; ++i) {
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

    Logger::InfoLog(LOC, "SharedOt_Online_Bench - Finished");
    if (kEvalType == ringoa::fss::EvalType::kIterSingleBatch) {
        Logger::ExportLogList("./data/logs/oa/sharedot_online_p" + ToString(party_id) + "_" + network);
    } else {
        Logger::ExportLogList("./data/logs/oa/sharedot_naive2_online_p" + ToString(party_id) + "_" + network);
    }
}

void RingOa_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "RingOa_Offline_Bench...");
    uint64_t iter = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;

    for (auto db_bitsize : db_bitsizes) {
        RingOaParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t            d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P   ass(d);
        ReplicatedSharing3P rss(d);
        RingOaKeyGenerator  gen(params, ass);
        FileIo              file_io;
        ShareIo             sh_io;
        KeyIo               key_io;

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("RingOa KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("RingOa OfflineSetUp");

        std::string key_path = kBenchOSPath + "ringoakey_d" + ToString(d);
        std::string db_path  = kBenchOSPath + "db_d" + ToString(d);
        std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

        for (uint64_t i = 0; i < iter; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::array<RingOaKey, 3> keys = gen.GenerateKeys();
            timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d));

            // Save keys
            key_io.SaveKey(key_path + "_0", keys[0]);
            key_io.SaveKey(key_path + "_1", keys[1]);
            key_io.SaveKey(key_path + "_2", keys[2]);
        }
        // Offline setup
        timer_mgr.SelectTimer(timer_off);
        timer_mgr.Start();
        gen.OfflineSetUp(iter, kBenchOSPath);
        rss.OfflineSetUp(kBenchOSPath + "prf");
        timer_mgr.Stop("OfflineSetUp(0) d=" + ToString(d));
        timer_mgr.PrintAllResults("Gen d=" + ToString(d), ringoa::MICROSECONDS, true);

        // // Generate the database and index
        // int32_t timer_data = timer_mgr.CreateNewTimer("OS DataGen");
        // timer_mgr.SelectTimer(timer_data);
        // timer_mgr.Start();
        // std::vector<uint64_t> database(1U << d);
        // for (size_t i = 0; i < database.size(); ++i) {
        //     database[i] = i;
        // }
        // uint64_t index = ass.GenerateRandomValue();
        // timer_mgr.Mark("DataGen d=" + ToString(d));

        // std::array<RepShareVec64, 3> database_sh = rss.ShareLocal(database);
        // std::array<RepShare64, 3>    index_sh    = rss.ShareLocal(index);
        // timer_mgr.Mark("ShareGen d=" + ToString(d));

        // // Save data
        // for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
        //     sh_io.SaveShare(db_path + "_" + ToString(p), database_sh[p]);
        //     sh_io.SaveShare(idx_path + "_" + ToString(p), index_sh[p]);
        // }
        // timer_mgr.Mark("ShareSave d=" + ToString(d));
        // timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), ringoa::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "RingOa_Offline_Bench - Finished");
    Logger::ExportLogList("./data/logs/oa/ringoa_offline_bench");
}

void RingOa_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "RingOa_Online_Bench...");
    uint64_t    iter     = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Helper that returns a task lambda for a given party_id
    auto MakeTask = [&](int party_id) {
        // Capture d, nu, params, and paths
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto db_bitsize : db_bitsizes) {
                RingOaParameters params(db_bitsize);
                params.PrintParameters();
                uint64_t d  = params.GetParameters().GetInputBitsize();
                uint64_t nu = params.GetParameters().GetTerminateBitsize();

                std::string key_path = kBenchOSPath + "ringoakey_d" + ToString(d);
                std::string db_path  = kBenchOSPath + "db_d" + ToString(d);
                std::string idx_path = kBenchOSPath + "idx_d" + ToString(d);

                // (1) Set up TimerManager and timers
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("RingOA SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("RingOA Eval");

                // (2) Begin SetUp timing
                timer_mgr.SelectTimer(timer_setup);
                timer_mgr.Start();

                // (3) Set up the replicated-sharing object and evaluator
                ReplicatedSharing3P rss(d);
                AdditiveSharing2P   ass_prev(d), ass_next(d);
                RingOaEvaluator     eval(params, rss, ass_prev, ass_next);
                Channels            chls(party_id, chl_prev, chl_next);
                RepShare64          result_sh;

                // (4) Load this party’s key
                RingOaKey key(party_id, params);
                KeyIo     key_io;
                key_io.LoadKey(key_path + "_" + ToString(party_id), key);

                // (5) Load this party’s shares of both databases and the index
                RepShareVec64              database_sh, _sh;
                RepShare64                 index_sh;
                std::vector<ringoa::block> uv_prev(1U << nu), uv_next(1U << nu);
                ShareIo                    sh_io;
                sh_io.LoadShare(db_path + "_" + ToString(party_id), database_sh);
                sh_io.LoadShare(idx_path + "_" + ToString(party_id), index_sh);

                // (6) Setup PRF keys
                eval.OnlineSetUp(party_id, kBenchOSPath);
                rss.OnlineSetUp(party_id, kBenchOSPath + "prf");

                // (7) Stop SetUp timer
                timer_mgr.Stop("SetUp d=" + ToString(d));

                // (8) Begin Eval timing
                timer_mgr.SelectTimer(timer_eval);

                // (9) iter Evaluate and measure each iteration
                for (uint64_t i = 0; i < iter; ++i) {
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

    Logger::InfoLog(LOC, "RingOa_Online_Bench - Finished");
    Logger::ExportLogList("./data/logs/oa/ringoa_online_p" + ToString(party_id) + "_" + network);
}

}    // namespace bench_ringoa
