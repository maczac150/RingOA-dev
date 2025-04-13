#include "fssfmi_bench.h"

#include <random>

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/fm_index/fssfmi.h"
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
#include "FssWM/wm/plain_wm.h"

namespace {

const std::string kCurrentPath     = fsswm::GetCurrentDirectory();
const std::string kBenchFssFMIPath = kCurrentPath + "/data/bench/fmi/";
const uint32_t    kFixedSeed       = 6;

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

std::vector<uint32_t> text_bitsizes     = {16, 20, 24};
std::vector<uint32_t> text_bitsizes_all = fsswm::CreateSequence(10, 27);
std::vector<uint32_t> query_sizes       = {16};

constexpr uint32_t repeat = 10;

}    // namespace

namespace bench_fsswm {

using fsswm::CreateSequence;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::Pow;
using fsswm::ShareType;
using fsswm::ThreePartyNetworkManager;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::fm_index::FssFMIEvaluator;
using fsswm::fm_index::FssFMIKey;
using fsswm::fm_index::FssFMIKeyGenerator;
using fsswm::fm_index::FssFMIParameters;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::Channels;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::RepShare, fsswm::sharing::RepShareVec, fsswm::sharing::RepShareMat;
using fsswm::sharing::ShareIo;
using fsswm::sharing::UIntVec, fsswm::sharing::UIntMat;
using fsswm::wm::FMIndex;
using fsswm::wm::KeyIo;

void FssFMI_Offline_Bench() {
    Logger::InfoLog(LOC, "FssFMI_Offline_Bench...");

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            FssFMIParameters          params(text_bitsize, query_size, ShareType::kBinary);
            uint32_t                  d  = params.GetDatabaseBitSize();
            uint32_t                  ds = params.GetDatabaseSize();
            uint32_t                  qs = params.GetQuerySize();
            AdditiveSharing2P         ass(d);
            BinarySharing2P           bss(d);
            ReplicatedSharing3P       rss(d);
            BinaryReplicatedSharing3P brss(d);
            FssFMIKeyGenerator        gen(params, ass, bss, brss);
            FileIo                    file_io;
            ShareIo                   sh_io;
            KeyIo                     key_io;

            TimerManager timer_mgr;
            int32_t      timer_keygen = timer_mgr.CreateNewTimer("FssFMI KeyGen");
            int32_t      timer_off    = timer_mgr.CreateNewTimer("FssFMI OfflineSetUp");

            std::string key_path   = kBenchFssFMIPath + "fssfmikey_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            std::string db0_path   = kBenchFssFMIPath + "db0_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            std::string db1_path   = kBenchFssFMIPath + "db1_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            std::string query_path = kBenchFssFMIPath + "query_d" + std::to_string(d) + "_qs" + std::to_string(qs);

            for (uint32_t i = 0; i < repeat; ++i) {
                timer_mgr.SelectTimer(timer_keygen);
                timer_mgr.Start();
                // Generate keys
                std::array<FssFMIKey, 3> keys = gen.GenerateKeys();
                // Save keys
                key_io.SaveKey(key_path + "_0", keys[0]);
                key_io.SaveKey(key_path + "_1", keys[1]);
                key_io.SaveKey(key_path + "_2", keys[2]);
                timer_mgr.Stop("KeyGen(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));

                timer_mgr.SelectTimer(timer_off);
                timer_mgr.Start();
                // Offline setup
                brss.OfflineSetUp(kBenchFssFMIPath + "prf");
                timer_mgr.Stop("OfflineSetUp(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));
            }
            timer_mgr.PrintAllResults("DataGen d=" + std::to_string(d) + " qs=" + std::to_string(qs), fsswm::MICROSECONDS, true);

            // // Generate the database and index
            // int32_t timer_data = timer_mgr.CreateNewTimer("FssFMI DataGen");
            // timer_mgr.SelectTimer(timer_data);
            // timer_mgr.Start();
            // std::string database = GenerateRandomString(ds - 2);
            // std::string query    = GenerateRandomString(qs);
            // timer_mgr.Mark("DataGen d=" + std::to_string(d) + " qs=" + std::to_string(qs));

            // // Generate the database and index
            // FMIndex fm(database);
            // timer_mgr.Mark("FMIndex d=" + std::to_string(d) + " qs=" + std::to_string(qs));

            // std::array<std::pair<RepShareMat, RepShareMat>, 3> db_sh    = gen.GenerateDatabaseShare(fm);
            // std::array<RepShareMat, 3>                         query_sh = gen.GenerateQueryShare(fm, query);
            // timer_mgr.Mark("ShareGen d=" + std::to_string(d) + " qs=" + std::to_string(qs));

            // // file_io.WriteToFile(db0_path, database);
            // // file_io.WriteToFile(db1_path, database);
            // // file_io.WriteToFile(query_path, query);
            // for (size_t p = 0; p < fsswm::sharing::kNumParties; ++p) {
            //     sh_io.SaveShare(db0_path + "_" + std::to_string(p), db_sh[p].first);
            //     sh_io.SaveShare(db1_path + "_" + std::to_string(p), db_sh[p].second);
            //     sh_io.SaveShare(query_path + "_" + std::to_string(p), query_sh[p]);
            // }
            // timer_mgr.Mark("ShareSave d=" + std::to_string(d) + " qs=" + std::to_string(qs));
            // timer_mgr.PrintCurrentResults("DataGen d=" + std::to_string(d) + " qs=" + std::to_string(qs), fsswm::MILLISECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FssFMI_Offline_Bench - Finished");
    FileIo io(".log");
    io.WriteToFile("./data/log/fssfmi_offline", Logger::GetLogList());
}

void FssFMI_Online_Bench(const oc::CLP &cmd) {
    Logger::InfoLog(LOC, "FssFMI_Online_Bench...");
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            FssFMIParameters params(text_bitsize, query_size, ShareType::kBinary);
            uint32_t         d  = params.GetDatabaseBitSize();
            uint32_t         qs = params.GetQuerySize();
            FileIo           file_io;
            ShareIo          sh_io;
            KeyIo            key_io;

            std::string key_path   = kBenchFssFMIPath + "fssfmikey_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            std::string db0_path   = kBenchFssFMIPath + "db0_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            std::string db1_path   = kBenchFssFMIPath + "db1_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            std::string query_path = kBenchFssFMIPath + "query_d" + std::to_string(d) + "_qs" + std::to_string(qs);
            // UIntVec     result;
            // std::string database;
            // std::string query;
            // file_io.ReadFromFile(db0_path, database);
            // file_io.ReadFromFile(db1_path, database);
            // file_io.ReadFromFile(query_path, query);

            // Define the tasks for each party
            ThreePartyNetworkManager net_mgr;

            // Party 0 task
            auto task_p0 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("FssFMI SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("FssFMI Eval");

                for (uint32_t i = 0; i < repeat; ++i) {
                    timer_mgr.SelectTimer(timer_setup);
                    timer_mgr.Start();
                    // Setup
                    ReplicatedSharing3P       rss(d);
                    BinaryReplicatedSharing3P brss(d);
                    FssFMIEvaluator           eval(params, rss, brss);
                    Channels                  chls(0, chl_prev, chl_next);
                    // Load keys
                    FssFMIKey key(0, params);
                    key_io.LoadKey(key_path + "_0", key);
                    // Load data
                    std::pair<RepShareMat, RepShareMat> db_sh;
                    RepShareMat                         query_sh;
                    sh_io.LoadShare(db0_path + "_0", db_sh.first);
                    sh_io.LoadShare(db1_path + "_0", db_sh.second);
                    sh_io.LoadShare(query_path + "_0", query_sh);
                    // Setup the PRF keys
                    brss.OnlineSetUp(0, kBenchFssFMIPath + "prf");
                    timer_mgr.Stop("SetUp(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));

                    timer_mgr.SelectTimer(timer_eval);
                    timer_mgr.Start();
                    // Evaluate
                    RepShareVec result_sh(qs);
                    eval.EvaluateLPM(chls, key, db_sh.first, db_sh.second, query_sh, result_sh);
                    timer_mgr.Stop("Eval(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));
                    Logger::InfoLog(LOC, "Total data sent: " + std::to_string(chls.GetStats()) + "bytes");
                    chls.ResetStats();
                }
                timer_mgr.PrintAllResults("d=" + std::to_string(d) + ", qs=" + std::to_string(query_size), fsswm::MILLISECONDS, true);
            };

            // Party 1 task
            auto task_p1 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("FssFMI SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("FssFMI Eval");

                for (uint32_t i = 0; i < repeat; ++i) {
                    timer_mgr.SelectTimer(timer_setup);
                    timer_mgr.Start();
                    // Setup
                    ReplicatedSharing3P       rss(d);
                    BinaryReplicatedSharing3P brss(d);
                    FssFMIEvaluator           eval(params, rss, brss);
                    Channels                  chls(1, chl_prev, chl_next);
                    // Load keys
                    FssFMIKey key(1, params);
                    key_io.LoadKey(key_path + "_1", key);
                    // Load data
                    std::pair<RepShareMat, RepShareMat> db_sh;
                    RepShareMat                         query_sh;
                    sh_io.LoadShare(db0_path + "_1", db_sh.first);
                    sh_io.LoadShare(db1_path + "_1", db_sh.second);
                    sh_io.LoadShare(query_path + "_1", query_sh);
                    // Setup the PRF keys
                    brss.OnlineSetUp(1, kBenchFssFMIPath + "prf");
                    timer_mgr.Stop("SetUp(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));

                    timer_mgr.SelectTimer(timer_eval);
                    timer_mgr.Start();
                    // Evaluate
                    RepShareVec result_sh(qs);
                    eval.EvaluateLPM(chls, key, db_sh.first, db_sh.second, query_sh, result_sh);
                    timer_mgr.Stop("Eval(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));
                    Logger::InfoLog(LOC, "Total data sent: " + std::to_string(chls.GetStats()) + "bytes");
                    chls.ResetStats();
                }
                timer_mgr.PrintAllResults("d=" + std::to_string(d) + ", qs=" + std::to_string(query_size), fsswm::MILLISECONDS, true);
            };

            // Party 2 task
            auto task_p2 = [&](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
                TimerManager timer_mgr;
                int32_t      timer_setup = timer_mgr.CreateNewTimer("FssFMI SetUp");
                int32_t      timer_eval  = timer_mgr.CreateNewTimer("FssFMI Eval");

                for (uint32_t i = 0; i < repeat; ++i) {
                    timer_mgr.SelectTimer(timer_setup);
                    timer_mgr.Start();
                    // Setup
                    ReplicatedSharing3P       rss(d);
                    BinaryReplicatedSharing3P brss(d);
                    FssFMIEvaluator           eval(params, rss, brss);
                    Channels                  chls(2, chl_prev, chl_next);
                    // Load keys
                    FssFMIKey key(2, params);
                    key_io.LoadKey(key_path + "_2", key);
                    // Load data
                    std::pair<RepShareMat, RepShareMat> db_sh;
                    RepShareMat                         query_sh;
                    sh_io.LoadShare(db0_path + "_2", db_sh.first);
                    sh_io.LoadShare(db1_path + "_2", db_sh.second);
                    sh_io.LoadShare(query_path + "_2", query_sh);
                    // Setup the PRF keys
                    brss.OnlineSetUp(2, kBenchFssFMIPath + "prf");
                    timer_mgr.Stop("SetUp(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));

                    timer_mgr.SelectTimer(timer_eval);
                    timer_mgr.Start();
                    // Evaluate
                    RepShareVec result_sh(qs);
                    eval.EvaluateLPM(chls, key, db_sh.first, db_sh.second, query_sh, result_sh);
                    timer_mgr.Stop("Eval(" + std::to_string(i) + ") d=" + std::to_string(d) + " qs=" + std::to_string(qs));
                    Logger::InfoLog(LOC, "Total data sent: " + std::to_string(chls.GetStats()) + "bytes");
                    chls.ResetStats();
                }
                timer_mgr.PrintAllResults("d=" + std::to_string(d) + ", qs=" + std::to_string(query_size), fsswm::MILLISECONDS, true);
            };

            // Configure network based on party ID and wait for completion
            net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
            net_mgr.WaitForCompletion();
        }
    }
    Logger::InfoLog(LOC, "FssFMI_Online_Bench - Finished");

    FileIo io(".log");

    if (party_id == 0) {
        io.WriteToFile("./data/log/fssfmi_online_p0_" + network, Logger::GetLogList());
    } else if (party_id == 1) {
        io.WriteToFile("./data/log/fssfmi_online_p1_" + network, Logger::GetLogList());
    } else if (party_id == 2) {
        io.WriteToFile("./data/log/fssfmi_online_p2_" + network, Logger::GetLogList());
    }
}

}    // namespace bench_fsswm
