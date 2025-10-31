#include "ofmi_bench.h"

#include <random>

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/fm_index/ofmi.h"
#include "RingOA/fm_index/ofmi_fsc.h"
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

const uint64_t kFixedSeed = 6;

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

std::string ReadFastaSequence(const std::string &fasta_path) {
    std::ifstream fin(fasta_path);
    if (!fin) {
        throw std::runtime_error("Failed to open file: " + fasta_path);
    }

    std::ostringstream seq;
    std::string        line;

    while (std::getline(fin, line)) {
        if (line.empty())
            continue;
        if (line[0] == '>')
            continue;    // skip header line
        seq << line;     // append sequence line
    }

    return seq.str();
}

}    // namespace

namespace bench_ringoa {

using ringoa::Channels;
using ringoa::CreateSequence;
using ringoa::FileIo;
using ringoa::Logger;
using ringoa::ThreePartyNetworkManager;
using ringoa::TimerManager;
using ringoa::ToString;
using ringoa::fm_index::OFMIEvaluator;
using ringoa::fm_index::OFMIFscEvaluator;
using ringoa::fm_index::OFMIFscKey;
using ringoa::fm_index::OFMIFscKeyGenerator;
using ringoa::fm_index::OFMIFscParameters;
using ringoa::fm_index::OFMIKey;
using ringoa::fm_index::OFMIKeyGenerator;
using ringoa::fm_index::OFMIParameters;
using ringoa::proto::KeyIo;
using ringoa::sharing::AdditiveSharing2P;
using ringoa::sharing::ReplicatedSharing3P;
using ringoa::sharing::RepShare64;
using ringoa::sharing::RepShareMat64;
using ringoa::sharing::RepShareVec64;
using ringoa::sharing::RepShareView64;
using ringoa::sharing::ShareIo;
using ringoa::wm::FMIndex;

void OFMI_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat        = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> text_bitsizes = SelectBitsizes(cmd);
    std::vector<uint64_t> query_sizes   = SelectQueryBitsize(cmd);

    Logger::InfoLog(LOC, "OFMI Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            OFMIParameters params(text_bitsize, query_size, 3);
            params.PrintParameters();

            uint64_t d  = params.GetDatabaseBitSize();
            uint64_t ds = params.GetDatabaseSize();
            uint64_t qs = params.GetQuerySize();

            AdditiveSharing2P   ass(d);
            ReplicatedSharing3P rss(d);
            OFMIKeyGenerator    gen(params, ass, rss);
            ShareIo             sh_io;
            KeyIo               key_io;
            TimerManager        timer_mgr;

            std::string key_path   = kBenchOfmiPath + "ofmikey_d" + ToString(d) + "_qs" + ToString(qs);
            std::string db_path    = kBenchOfmiPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
            std::string query_path = kBenchOfmiPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

            {    // KeyGen
                const std::string timer_name = "OFMI KeyGen";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    std::array<OFMIKey, 3> keys = gen.GenerateKeys();
                    timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=" + ToString(i));
                    for (size_t p = 0; p < 3; ++p)
                        key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
                }
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
            }

            {    // OfflineSetUp
                const std::string timer_name = "OFMI OfflineSetUp";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                timer_mgr.Start();
                rss.OfflineSetUp(kBenchOfmiPath + "prf");
                gen.OfflineSetUp(kBenchOfmiPath);
                timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
            }

            {    // DataGen
                const std::string timer_name = "OFMI DataGen";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                timer_mgr.Start();

                std::string database, query;
                database = GenerateRandomString(ds - 2);
                query    = GenerateRandomString(qs);
                timer_mgr.Mark("DataGen d=" + ToString(d) + " qs=" + ToString(qs));

                FMIndex fm(database);
                timer_mgr.Mark("FMIndex d=" + ToString(d) + " qs=" + ToString(qs));

                std::array<RepShareMat64, 3> db_sh    = gen.GenerateDatabaseU64Share(fm);
                std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
                timer_mgr.Mark("ShareGen d=" + ToString(d) + " qs=" + ToString(qs));

                for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                    sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
                    sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
                }
                timer_mgr.Mark("ShareSave d=" + ToString(d) + " qs=" + ToString(qs));
                timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MILLISECONDS, true);
            }
        }
    }

    Logger::InfoLog(LOC, "OFMI Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogOfmiPath + "ofmi_offline", true);
}

void OFMI_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat        = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id      = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network       = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> text_bitsizes = SelectBitsizes(cmd);
    std::vector<uint64_t> query_sizes   = SelectQueryBitsize(cmd);

    Logger::InfoLog(LOC, "OFMI Online Benchmark started (repeat=" + ToString(repeat) + ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto text_bitsize : text_bitsizes) {
                for (auto query_size : query_sizes) {
                    OFMIParameters params(text_bitsize, query_size);
                    params.PrintParameters();

                    uint64_t d  = params.GetDatabaseBitSize();
                    uint64_t qs = params.GetQuerySize();
                    uint64_t nu = params.GetOWMParameters().GetOaParameters().GetParameters().GetTerminateBitsize();

                    std::string key_path   = kBenchOfmiPath + "ofmikey_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string db_path    = kBenchOfmiPath + "db_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string query_path = kBenchOfmiPath + "query_d" + ToString(d) + "_qs" + ToString(qs);

                    TimerManager timer_mgr;
                    int32_t      id_setup = timer_mgr.CreateNewTimer("OFMI OnlineSetUp " + ptag);
                    int32_t      id_eval  = timer_mgr.CreateNewTimer("OFMI Eval " + ptag);

                    timer_mgr.SelectTimer(id_setup);
                    timer_mgr.Start();
                    ReplicatedSharing3P        rss(d);
                    AdditiveSharing2P          ass_prev(d), ass_next(d);
                    OFMIEvaluator              eval(params, rss, ass_prev, ass_next);
                    Channels                   chls(p, chl_prev, chl_next);
                    std::vector<ringoa::block> uv_prev(1ULL << nu), uv_next(1ULL << nu);
                    OFMIKey                    key(p, params);
                    KeyIo                      key_io;
                    key_io.LoadKey(key_path + "_" + ToString(p), key);
                    RepShareMat64 db_sh;
                    RepShareMat64 query_sh;
                    ShareIo       sh_io;
                    sh_io.LoadShare(db_path + "_" + ToString(p), db_sh);
                    sh_io.LoadShare(query_path + "_" + ToString(p), query_sh);
                    eval.OnlineSetUp(p, kBenchOfmiPath);
                    rss.OnlineSetUp(p, kBenchOfmiPath + "prf");
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
                        ass_prev.ResetTripleIndex();
                        ass_next.ResetTripleIndex();
                    }
                    timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
                }
            }
        };
    };

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, MakeTask(0), MakeTask(1), MakeTask(2));
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OFMI Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogOfmiPath + "ofmi_online_p" + ToString(party_id) + "_" + network, true);
}

void OFMI_Fsc_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat        = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> text_bitsizes = SelectBitsizes(cmd);
    std::vector<uint64_t> query_sizes   = SelectQueryBitsize(cmd);

    Logger::InfoLog(LOC, "OFMI (FSC) Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto text_bitsize : text_bitsizes) {
        for (auto query_size : query_sizes) {
            OFMIFscParameters params(text_bitsize, query_size, 3);
            params.PrintParameters();

            uint64_t d  = params.GetDatabaseBitSize();
            uint64_t ds = params.GetDatabaseSize();
            uint64_t qs = params.GetQuerySize();

            AdditiveSharing2P   ass(d);
            ReplicatedSharing3P rss(d);
            OFMIFscKeyGenerator gen(params, ass, rss);
            ShareIo             sh_io;
            KeyIo               key_io;
            TimerManager        timer_mgr;

            std::string key_path   = kBenchOfmiPath + "ofmifsckey_d" + ToString(d) + "_qs" + ToString(qs);
            std::string db_path    = kBenchOfmiPath + "dbfsc_d" + ToString(d) + "_qs" + ToString(qs);
            std::string aux_path   = kBenchOfmiPath + "auxfsc_d" + ToString(d) + "_qs" + ToString(qs);
            std::string query_path = kBenchOfmiPath + "queryfsc_d" + ToString(d) + "_qs" + ToString(qs);

            std::array<bool, 3> v_sign = {};

            {    // DataGen
                const std::string timer_name = "OFMI (FSC) DataGen";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                timer_mgr.Start();

                std::string database, query;
                database = GenerateRandomString(ds - 2);
                query    = GenerateRandomString(qs);
                timer_mgr.Mark("DataGen d=" + ToString(d) + " qs=" + ToString(qs));

                FMIndex fm(database);
                timer_mgr.Mark("FMIndex d=" + ToString(d) + " qs=" + ToString(qs));

                std::array<RepShareMat64, 3> db_sh;
                std::array<RepShareVec64, 3> aux_sh;
                gen.GenerateDatabaseU64Share(fm, db_sh, aux_sh, v_sign);
                std::array<RepShareMat64, 3> query_sh = gen.GenerateQueryU64Share(fm, query);
                timer_mgr.Mark("ShareGen d=" + ToString(d) + " qs=" + ToString(qs));

                for (size_t p = 0; p < ringoa::sharing::kThreeParties; ++p) {
                    sh_io.SaveShare(db_path + "_" + ToString(p), db_sh[p]);
                    sh_io.SaveShare(query_path + "_" + ToString(p), query_sh[p]);
                    sh_io.SaveShare(aux_path + "_" + ToString(p), aux_sh[p]);
                }
                timer_mgr.Mark("ShareSave d=" + ToString(d) + " qs=" + ToString(qs));
                timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MILLISECONDS, true);
            }

            {    // KeyGen
                const std::string timer_name = "OFMI (FSC) KeyGen";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                for (uint64_t i = 0; i < repeat; ++i) {
                    timer_mgr.Start();
                    std::array<OFMIFscKey, 3> keys = gen.GenerateKeys(v_sign);
                    timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=" + ToString(i));
                    for (size_t p = 0; p < 3; ++p)
                        key_io.SaveKey(key_path + "_" + ToString(p), keys[p]);
                }
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
            }

            {    // OfflineSetUp
                const std::string timer_name = "OFMI (FSC) OfflineSetUp";
                int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
                timer_mgr.SelectTimer(timer_id);
                timer_mgr.Start();
                rss.OfflineSetUp(kBenchOfmiPath + "prf");
                timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
            }
        }
    }

    Logger::InfoLog(LOC, "OFMI Offline Benchmark completed");
    Logger::ExportLogListAndClear(kLogOfmiPath + "ofmi_fsc_offline", true);
}

void OFMI_Fsc_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat        = cmd.getOr("repeat", kRepeatDefault);
    int                   party_id      = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string           network       = cmd.isSet("network") ? cmd.get<std::string>("network") : "";
    std::vector<uint64_t> text_bitsizes = SelectBitsizes(cmd);
    std::vector<uint64_t> query_sizes   = SelectQueryBitsize(cmd);

    Logger::InfoLog(LOC, "OFMI (FSC) Online Benchmark started (repeat=" + ToString(repeat) + ", party=" + ToString(party_id) + ")");

    auto MakeTask = [&](int p) {
        const std::string ptag = "(P" + ToString(p) + ")";
        return [=](osuCrypto::Channel &chl_next, osuCrypto::Channel &chl_prev) {
            for (auto text_bitsize : text_bitsizes) {
                for (auto query_size : query_sizes) {
                    OFMIFscParameters params(text_bitsize, query_size);
                    params.PrintParameters();

                    uint64_t d  = params.GetDatabaseBitSize();
                    uint64_t qs = params.GetQuerySize();
                    uint64_t nu = params.GetOWMFscParameters().GetOaParameters().GetParameters().GetTerminateBitsize();

                    std::string key_path   = kBenchOfmiPath + "ofmifsckey_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string db_path    = kBenchOfmiPath + "dbfsc_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string aux_path   = kBenchOfmiPath + "auxfsc_d" + ToString(d) + "_qs" + ToString(qs);
                    std::string query_path = kBenchOfmiPath + "queryfsc_d" + ToString(d) + "_qs" + ToString(qs);

                    TimerManager timer_mgr;
                    int32_t      id_setup = timer_mgr.CreateNewTimer("OFMI (FSC) OnlineSetUp " + ptag);
                    int32_t      id_eval  = timer_mgr.CreateNewTimer("OFMI (FSC) Eval " + ptag);

                    timer_mgr.SelectTimer(id_setup);
                    timer_mgr.Start();
                    ReplicatedSharing3P        rss(d);
                    AdditiveSharing2P          ass_prev(d), ass_next(d);
                    OFMIFscEvaluator           eval(params, rss, ass_prev, ass_next);
                    Channels                   chls(p, chl_prev, chl_next);
                    std::vector<ringoa::block> uv_prev(1ULL << nu), uv_next(1ULL << nu);
                    OFMIFscKey                 key(p, params);
                    KeyIo                      key_io;
                    key_io.LoadKey(key_path + "_" + ToString(p), key);
                    RepShareMat64 db_sh;
                    RepShareVec64 aux_sh;
                    RepShareMat64 query_sh;
                    ShareIo       sh_io;
                    sh_io.LoadShare(db_path + "_" + ToString(p), db_sh);
                    sh_io.LoadShare(aux_path + "_" + ToString(p), aux_sh);
                    sh_io.LoadShare(query_path + "_" + ToString(p), query_sh);
                    rss.OnlineSetUp(p, kBenchOfmiPath + "prf");
                    timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=0");
                    timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);

                    timer_mgr.SelectTimer(id_eval);
                    for (uint64_t i = 0; i < repeat; ++i) {
                        timer_mgr.Start();
                        RepShareVec64 result_sh(qs);
                        eval.EvaluateLPM_Parallel(chls, key, uv_prev, uv_next, db_sh, RepShareView64(aux_sh), query_sh, result_sh);
                        timer_mgr.Stop("d=" + ToString(d) + " qs=" + ToString(qs) + " iter=" + ToString(i));
                        if (i < 2)
                            Logger::InfoLog(LOC, "d=" + ToString(d) + " qs=" + ToString(qs) + " total_data_sent=" + ToString(chls.GetStats()) + " bytes");
                        chls.ResetStats();
                        ass_prev.ResetTripleIndex();
                        ass_next.ResetTripleIndex();
                    }
                    timer_mgr.PrintCurrentResults("d=" + ToString(d) + " qs=" + ToString(qs), ringoa::MICROSECONDS, true);
                }
            }
        };
    };

    ThreePartyNetworkManager net_mgr;
    net_mgr.AutoConfigure(party_id, MakeTask(0), MakeTask(1), MakeTask(2));
    net_mgr.WaitForCompletion();

    Logger::InfoLog(LOC, "OFMI Online Benchmark completed");
    Logger::ExportLogListAndClear(kLogOfmiPath + "ofmi_online_p" + ToString(party_id) + "_" + network, true);
}

}    // namespace bench_ringoa
