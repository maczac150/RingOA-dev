#include "fssfmi_bench.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/fm_index/fssfmi.h"
#include "FssWM/fm_index/sotfmi.h"
#include "FssWM/protocol/key_io.h"
#include "FssWM/sharing/additive_2p.h"
#include "FssWM/sharing/additive_3p.h"
#include "FssWM/sharing/binary_2p.h"
#include "FssWM/sharing/binary_3p.h"
#include "FssWM/sharing/share_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/network.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/plain_wm.h"

namespace {

const std::string kCurrentPath     = fsswm::GetCurrentDirectory();
const std::string kBenchFssFMIPath = kCurrentPath + "/data/bench/fmi/";
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

std::vector<uint64_t> text_bitsizes = fsswm::CreateSequence(10, 31);
// std::vector<uint64_t> text_bitsizes = {24};
std::vector<uint64_t> query_sizes = {16};

constexpr uint64_t kIterDefault = 10;

}    // namespace

namespace bench_fsswm {

using fsswm::Channels;
using fsswm::CreateSequence;
using fsswm::FileIo;
using fsswm::Logger;
using fsswm::ThreePartyNetworkManager;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::fm_index::FssFMIEvaluator;
using fsswm::fm_index::FssFMIKey;
using fsswm::fm_index::FssFMIKeyGenerator;
using fsswm::fm_index::FssFMIParameters;
using fsswm::fm_index::SotFMIEvaluator;
using fsswm::fm_index::SotFMIKey;
using fsswm::fm_index::SotFMIKeyGenerator;
using fsswm::fm_index::SotFMIParameters;
using fsswm::proto::KeyIo;
using fsswm::sharing::AdditiveSharing2P;
using fsswm::sharing::BinaryReplicatedSharing3P;
using fsswm::sharing::BinarySharing2P;
using fsswm::sharing::ReplicatedSharing3P;
using fsswm::sharing::RepShare64, fsswm::sharing::RepShareBlock;
using fsswm::sharing::RepShareMat64, fsswm::sharing::RepShareMatBlock;
using fsswm::sharing::RepShareVec64, fsswm::sharing::RepShareVecBlock;
using fsswm::sharing::ShareIo;
using fsswm::wm::FMIndex;

void SotFMI_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "SotFMI_Offline_Bench...");
    uint64_t iter = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            SotFMIParameters    params(text_bitsize, query_size, 3);
            uint64_t            d  = params.GetDatabaseBitSize();
            uint64_t            ds = params.GetDatabaseSize();
            uint64_t            qs = params.GetQuerySize();
            AdditiveSharing2P   ass(d);
            ReplicatedSharing3P rss(d);
            SotFMIKeyGenerator  gen(params, ass, rss);
            FileIo              file_io;
            ShareIo             sh_io;
            KeyIo               key_io;

            TimerManager timer_mgr;
            int32_t      timer_keygen = timer_mgr.CreateNewTimer("SotFMI KeyGen");
            int32_t      timer_off    = timer_mgr.CreateNewTimer("SotFMI OfflineSetUp");

            std::string key_path   = kBenchFssFMIPath + "sotfmikey_d" + ToString(d) + "_qs" + ToString(qs);
            std::string db_path    = kBenchFssFMIPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
            std::string query_path = kBenchFssFMIPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

            for (uint64_t i = 0; i < iter; ++i) {
                timer_mgr.SelectTimer(timer_keygen);
                timer_mgr.Start();
                // Generate keys
                std::array<SotFMIKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d) + " qs=" + ToString(qs));
                // Save keys
                key_io.SaveKey(key_path + "_0", keys[0]);
                key_io.SaveKey(key_path + "_1", keys[1]);
                key_io.SaveKey(key_path + "_2", keys[2]);
            }

            timer_mgr.SelectTimer(timer_off);
            timer_mgr.Start();
            // Offline setup
            rss.OfflineSetUp(kBenchFssFMIPath + "prf");
            timer_mgr.Stop("OfflineSetUp(0) d=" + ToString(d) + " qs=" + ToString(qs));
            timer_mgr.PrintAllResults("Gen d=" + ToString(d) + " qs=" + ToString(qs), fsswm::MICROSECONDS, true);

            // Generate the database and index
            int32_t timer_data = timer_mgr.CreateNewTimer("SotFMI DataGen");
            timer_mgr.SelectTimer(timer_data);
            timer_mgr.Start();
            std::string database = GenerateRandomString(ds - 2);
            std::string query    = GenerateRandomString(qs);
            timer_mgr.Mark("DataGen d=" + ToString(d) + " qs=" + ToString(qs));

            // // Generate the database and index
            // FMIndex fm(database);
            // timer_mgr.Mark("FMIndex d=" + ToString(d) + " qs=" + ToString(qs));

            // std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(fm);
            // std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
            // timer_mgr.Mark("ShareGen d=" + ToString(d) + " qs=" + ToString(qs));

            // for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            //     sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            //     sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
            // }
            // timer_mgr.Mark("ShareSave d=" + ToString(d) + " qs=" + ToString(qs));
            // timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d) + " qs=" + ToString(qs), fsswm::MILLISECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "SotFMI_Offline_Bench - Finished");
    Logger::ExportLogList("./data/logs/fssfmi/sotfmi_offline");
}

void SotFMI_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "SotFMI_Online_Bench...");
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    uint64_t    iter     = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Factory to create a per-party task
    auto MakeTask = [&](int party_id_inner) {
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto text_bitsize : text_bitsizes) {
                for (auto query_size : query_sizes) {
                    SotFMIParameters params(text_bitsize, query_size);
                    uint64_t         d  = params.GetDatabaseBitSize();
                    uint64_t         qs = params.GetQuerySize();

                    std::string key_path   = kBenchFssFMIPath + "sotfmikey_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string db_path    = kBenchFssFMIPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string query_path = kBenchFssFMIPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

                    TimerManager timer_mgr;
                    int32_t      timer_setup = timer_mgr.CreateNewTimer("SotFMI SetUp");
                    int32_t      timer_eval  = timer_mgr.CreateNewTimer("SotFMI Eval");

                    timer_mgr.SelectTimer(timer_setup);
                    timer_mgr.Start();
                    // Setup
                    ReplicatedSharing3P   rss(d);
                    AdditiveSharing2P     ass(d);
                    SotFMIEvaluator       eval(params, rss, ass);
                    Channels              chls(party_id_inner, chl_prev, chl_next);
                    std::vector<uint64_t> uv_prev(1U << d), uv_next(1U << d);
                    // Load keys
                    SotFMIKey key(party_id_inner, params);
                    KeyIo     key_io;
                    key_io.LoadKey(key_path + "_" + ToString(party_id_inner), key);
                    // Load data
                    RepShareMat64 db_sh;
                    RepShareMat64 query_sh;
                    ShareIo       sh_io;
                    sh_io.LoadShare(db_path + "_" + ToString(party_id_inner), db_sh);
                    sh_io.LoadShare(query_path + "_" + ToString(party_id_inner), query_sh);
                    // Setup the PRF keys
                    rss.OnlineSetUp(party_id, kBenchFssFMIPath + "prf");
                    timer_mgr.Stop("SetUp d=" + ToString(d) + " qs=" + ToString(qs));

                    timer_mgr.SelectTimer(timer_eval);
                    for (uint64_t i = 0; i < iter; ++i) {
                        // Evaluate
                        timer_mgr.Start();
                        RepShareVec64 result_sh(qs);
                        eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, query_sh, result_sh);
                        timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d) + " qs=" + ToString(qs));
                        if (i < 2)
                            Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + "bytes");
                        chls.ResetStats();
                    }

                    timer_mgr.PrintAllResults("d=" + ToString(d) + ", qs=" + ToString(qs), fsswm::MILLISECONDS, true);
                }
            }
        };
    };

    auto task_p0 = MakeTask(0);
    auto task_p1 = MakeTask(1);
    auto task_p2 = MakeTask(2);

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "SotFMI_Online_Bench - Finished");
    Logger::ExportLogList("./data/logs/fssfmi/sotfmi_online_p" + ToString(party_id) + "_" + network);
}

void FssFMI_Offline_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "FssFMI_Offline_Bench...");
    uint64_t iter = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            FssFMIParameters    params(text_bitsize, query_size, 3);
            uint64_t            d  = params.GetDatabaseBitSize();
            uint64_t            ds = params.GetDatabaseSize();
            uint64_t            qs = params.GetQuerySize();
            AdditiveSharing2P   ass(d);
            ReplicatedSharing3P rss(d);
            FssFMIKeyGenerator  gen(params, ass, rss);
            FileIo              file_io;
            ShareIo             sh_io;
            KeyIo               key_io;

            TimerManager timer_mgr;
            int32_t      timer_keygen = timer_mgr.CreateNewTimer("FssFMI KeyGen");
            int32_t      timer_off    = timer_mgr.CreateNewTimer("FssFMI OfflineSetUp");

            std::string key_path   = kBenchFssFMIPath + "fssfmikey_d" + ToString(d) + "_qs" + ToString(qs);
            std::string db_path    = kBenchFssFMIPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
            std::string query_path = kBenchFssFMIPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

            for (uint64_t i = 0; i < iter; ++i) {
                timer_mgr.SelectTimer(timer_keygen);
                timer_mgr.Start();
                // Generate keys
                std::array<FssFMIKey, 3> keys = gen.GenerateKeys();
                timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d) + " qs=" + ToString(qs));
                // Save keys
                key_io.SaveKey(key_path + "_0", keys[0]);
                key_io.SaveKey(key_path + "_1", keys[1]);
                key_io.SaveKey(key_path + "_2", keys[2]);
            }

            timer_mgr.SelectTimer(timer_off);
            timer_mgr.Start();
            // Offline setup
            rss.OfflineSetUp(kBenchFssFMIPath + "prf");
            gen.OfflineSetUp(kBenchFssFMIPath);
            timer_mgr.Stop("OfflineSetUp(0) d=" + ToString(d) + " qs=" + ToString(qs));
            timer_mgr.PrintAllResults("Gen d=" + ToString(d) + " qs=" + ToString(qs), fsswm::MICROSECONDS, true);

            // Generate the database and index
            int32_t timer_data = timer_mgr.CreateNewTimer("FssFMI DataGen");
            timer_mgr.SelectTimer(timer_data);
            timer_mgr.Start();
            std::string database = GenerateRandomString(ds - 2);
            std::string query    = GenerateRandomString(qs);
            timer_mgr.Mark("DataGen d=" + ToString(d) + " qs=" + ToString(qs));

            // // Generate the database and index
            // FMIndex fm(database);
            // timer_mgr.Mark("FMIndex d=" + ToString(d) + " qs=" + ToString(qs));

            // std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(fm);
            // std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
            // timer_mgr.Mark("ShareGen d=" + ToString(d) + " qs=" + ToString(qs));

            // for (size_t p = 0; p < fsswm::sharing::kThreeParties; ++p) {
            //     sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
            //     sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
            // }
            // timer_mgr.Mark("ShareSave d=" + ToString(d) + " qs=" + ToString(qs));
            // timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d) + " qs=" + ToString(qs), fsswm::MILLISECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FssFMI_Offline_Bench - Finished");
    Logger::ExportLogList("./data/logs/fssfmi/fssfmi_offline");
}

void FssFMI_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "FssFMI_Online_Bench...");
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    uint64_t    iter     = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    // Factory to create a per-party task
    auto MakeTask = [&](int party_id_inner) {
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto text_bitsize : text_bitsizes) {
                for (auto query_size : query_sizes) {
                    FssFMIParameters params(text_bitsize, query_size);
                    uint64_t         d  = params.GetDatabaseBitSize();
                    uint64_t         qs = params.GetQuerySize();
                    uint64_t         nu = params.GetFssWMParameters().GetOaParameters().GetParameters().GetTerminateBitsize();

                    std::string key_path   = kBenchFssFMIPath + "fssfmikey_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string db_path    = kBenchFssFMIPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string query_path = kBenchFssFMIPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

                    TimerManager timer_mgr;
                    int32_t      timer_setup = timer_mgr.CreateNewTimer("FssFMI SetUp");
                    int32_t      timer_eval  = timer_mgr.CreateNewTimer("FssFMI Eval");

                    timer_mgr.SelectTimer(timer_setup);
                    timer_mgr.Start();
                    // Setup
                    ReplicatedSharing3P       rss(d);
                    AdditiveSharing2P         ass_prev(d), ass_next(d);
                    FssFMIEvaluator           eval(params, rss, ass_prev, ass_next);
                    Channels                  chls(party_id_inner, chl_prev, chl_next);
                    std::vector<fsswm::block> uv_prev(1U << nu), uv_next(1U << nu);
                    // Load keys
                    FssFMIKey key(party_id_inner, params);
                    KeyIo     key_io;
                    key_io.LoadKey(key_path + "_" + ToString(party_id_inner), key);
                    // Load data
                    RepShareMat64 db_sh;
                    RepShareMat64 query_sh;
                    ShareIo       sh_io;
                    sh_io.LoadShare(db_path + "_" + ToString(party_id_inner), db_sh);
                    sh_io.LoadShare(query_path + "_" + ToString(party_id_inner), query_sh);
                    // Setup the PRF keys
                    eval.OnlineSetUp(party_id_inner, kBenchFssFMIPath);
                    rss.OnlineSetUp(party_id_inner, kBenchFssFMIPath + "prf");
                    timer_mgr.Stop("SetUp d=" + ToString(d) + " qs=" + ToString(qs));
                    
                    timer_mgr.SelectTimer(timer_eval);
                    for (uint64_t i = 0; i < iter; ++i) {
                        // Evaluate
                        timer_mgr.Start();
                        RepShareVec64 result_sh(qs);
                        eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, query_sh, result_sh);
                        timer_mgr.Stop("Eval(" + ToString(i) + ") d=" + ToString(d) + " qs=" + ToString(qs));
                        if (i < 2)
                            Logger::InfoLog(LOC, "Total data sent: " + ToString(chls.GetStats()) + "bytes");
                        chls.ResetStats();
                        ass_prev.ResetTripleIndex();
                        ass_next.ResetTripleIndex();
                    }

                    timer_mgr.PrintAllResults("d=" + ToString(d) + ", qs=" + ToString(qs), fsswm::MILLISECONDS, true);
                }
            }
        };
    };

    auto task_p0 = MakeTask(0);
    auto task_p1 = MakeTask(1);
    auto task_p2 = MakeTask(2);

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, task_p0, task_p1, task_p2);
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "FssFMI_Online_Bench - Finished");
    Logger::ExportLogList("./data/logs/fssfmi/fssfmi_online_p" + ToString(party_id) + "_" + network);
}

}    // namespace bench_fsswm
