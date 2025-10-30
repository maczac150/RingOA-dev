#include "dpf_pir_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/protocol/dpf_pir.h"
#include "RingOA/protocol/key_io.h"
#include "RingOA/sharing/additive_2p.h"
#include "RingOA/utils/file_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/network.h"
#include "RingOA/utils/rng.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace {

const std::string kCurrentPath  = ringoa::GetCurrentDirectory();
const std::string kBenchPirPath = kCurrentPath + "/data/bench/pir/";

std::vector<uint64_t> db_bitsizes = {12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
// std::vector<uint64_t> db_bitsizes = ringoa::CreateSequence(10, 31);

constexpr uint64_t kRepeatDefault = 10;

}    // namespace

namespace bench_ringoa {

using ringoa::block;
using ringoa::FileIo;
using ringoa::FormatType;
using ringoa::GlobalRng;
using ringoa::Logger;
using ringoa::Mod2N;
using ringoa::TimerManager;
using ringoa::ToString, ringoa::Format;
using ringoa::TwoPartyNetworkManager;
using ringoa::proto::DpfPirEvaluator;
using ringoa::proto::DpfPirKey;
using ringoa::proto::DpfPirKeyGenerator;
using ringoa::proto::DpfPirParameters;
using ringoa::proto::KeyIo;
using ringoa::sharing::AdditiveSharing2P;

void DpfPir_Offline_Bench(const osuCrypto::CLP &cmd) {
    uint64_t repeat = cmd.getOr("repeat", kRepeatDefault);

    Logger::InfoLog(LOC, "DpfPir Offline Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto db_bitsize : db_bitsizes) {
        DpfPirParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t           d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P  ass(d);
        DpfPirKeyGenerator gen(params, ass);
        FileIo             file_io;
        KeyIo              key_io;

        std::string  key_path = kBenchPirPath + "dpfpirkey_d" + ToString(d);
        std::string  db_path  = kBenchPirPath + "db_d" + ToString(d);
        std::string  idx_path = kBenchPirPath + "idx_d" + ToString(d);
        TimerManager timer_mgr;

        //
        // 1. KeyGen timing
        //
        {
            // timer_name is a stable label for what we're measuring
            const std::string timer_name = "DpfPir KeyGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();

                // Generate keys
                std::pair<DpfPirKey, DpfPirKey> keys = gen.GenerateKeys();

                timer_mgr.Stop(
                    "d=" + ToString(d) +
                    " iter=" + ToString(i));

                // Save keys
                key_io.SaveKey(key_path + "_0", keys.first);
                key_io.SaveKey(key_path + "_1", keys.second);
            }

            // Summarize KeyGen timing for this d
            const std::string summary_msg =
                "KeyGen d=" + ToString(d);

            timer_mgr.PrintCurrentResults(
                summary_msg,
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }

        //
        // 2. OfflineSetUp timing
        //
        {
            const std::string timer_name = "DpfPir OfflineSetUp";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            // For offline setup we measure once, not per-iter.
            timer_mgr.Start();

            // Offline setup (likely precomputation material shared by parties)
            gen.OfflineSetUp(repeat, kBenchPirPath);

            timer_mgr.Stop(
                "d=" + ToString(d) +
                " iter=0");

            const std::string summary_msg =
                "OfflineSetUp d=" + ToString(d);

            timer_mgr.PrintCurrentResults(
                summary_msg,
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }

        //
        // 3. Data generation + secret sharing timing
        //
        {
            const std::string timer_name = "DpfPir DataGen";
            int32_t           timer_id   = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            // Start timing
            timer_mgr.Start();

            // Build the database of size 2^d
            std::vector<uint64_t> database(1ULL << d);
            for (size_t i = 0; i < database.size(); ++i) {
                database[i] = static_cast<uint64_t>(i);
            }

            uint64_t index = ass.GenerateRandomValue();
            timer_mgr.Mark("DataGen d=" + ToString(d));

            // Secret-share the query index
            std::pair<uint64_t, uint64_t> idx_sh = ass.Share(index);
            timer_mgr.Mark("ShareGen d=" + ToString(d));

            // Save database and shares
            file_io.WriteBinary(idx_path + "_0", idx_sh.first);
            file_io.WriteBinary(idx_path + "_1", idx_sh.second);
            file_io.WriteBinary(db_path, database);
            timer_mgr.Mark("ShareSave d=" + ToString(d));

            // Stop timing block
            timer_mgr.Stop(
                "d=" + ToString(d) +
                " iter=0");

            const std::string summary_msg =
                "DataGen d=" + ToString(d);

            // Note: in DataGen we might care about milliseconds instead of µs
            timer_mgr.PrintCurrentResults(
                summary_msg,
                ringoa::TimeUnit::MILLISECONDS,
                /*show_details=*/true);
        }
    }
    Logger::InfoLog(LOC, "DpfPir_Offline_Bench - Finished");
}

void DpfPir_Online_Bench(const osuCrypto::CLP &cmd) {
    uint64_t repeat = cmd.getOr("repeat", kRepeatDefault);

    // party_id: which role to run locally (0 = server, 1 = client, -1 = auto)
    int party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;

    Logger::InfoLog(LOC, "DpfPir Online Benchmark started (repeat=" + ToString(repeat) +
                             ", party=" + ToString(party_id) + ")");

    for (auto db_bitsize : db_bitsizes) {
        DpfPirParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t d  = params.GetDatabaseSize();
        uint64_t nu = params.GetParameters().GetTerminateBitsize();
        KeyIo    key_io;
        FileIo   file_io;

        // Prepare file paths (must match Offline bench output)
        std::string key_path = kBenchPirPath + "dpfpirkey_d" + ToString(d);
        std::string idx_path = kBenchPirPath + "idx_d" + ToString(d);
        std::string db_path  = kBenchPirPath + "db_d" + ToString(d);

        // Load shared database into memory (both parties need it)
        std::vector<uint64_t> database;
        file_io.ReadBinary(db_path, database);

        uint64_t idx_0 = 0, idx_1 = 0;
        uint64_t y_0 = 0, y_1 = 0;

        // Start network communication
        TwoPartyNetworkManager net_mgr("DpfPir_Online_Bench");

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("DpfPir OnlineSetUp (P0)");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("DpfPir Eval (P0)");

            timer_mgr.SelectTimer(timer_setup);
            timer_mgr.Start();

            AdditiveSharing2P ss(d);
            DpfPirEvaluator   eval(params, ss);
            // Load keys
            DpfPirKey key_0(0, params);
            key_io.LoadKey(key_path + "_0", key_0);

            // Load input
            file_io.ReadBinary(idx_path + "_0", idx_0);

            std::vector<block> uv(1U << nu);

            // Online Setup
            eval.OnlineSetUp(0, kBenchPirPath);
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");

            timer_mgr.SelectTimer(timer_eval);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                // Evaluate
                y_0 = eval.Evaluate(chl, key_0, uv, database, idx_0);
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));

                if (i < 2)
                    Logger::InfoLog(LOC, "d=" + ToString(d) +
                                             " total_data_sent=" + ToString(chl.getTotalDataSent()) + " bytes");
                chl.resetStats();
            }
            timer_mgr.PrintAllResults("Eval d=" + ToString(d), ringoa::MICROSECONDS, true);
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("DpfPir OnlineSetUp (P1)");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("DpfPir Eval (P1)");

            timer_mgr.SelectTimer(timer_setup);
            timer_mgr.Start();

            AdditiveSharing2P ss(d);
            DpfPirEvaluator   eval(params, ss);
            // Load keys
            DpfPirKey key_1(1, params);
            key_io.LoadKey(key_path + "_1", key_1);

            // Load input
            file_io.ReadBinary(idx_path + "_1", idx_1);

            std::vector<block> uv(1U << nu);

            // Online Setup
            eval.OnlineSetUp(0, kBenchPirPath);
            timer_mgr.Stop("d=" + ToString(d) + " iter=0");

            timer_mgr.SelectTimer(timer_eval);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                // Evaluate
                y_1 = eval.Evaluate(chl, key_1, uv, database, idx_1);
                timer_mgr.Stop("d=" + ToString(d) + " iter=" + ToString(i));

                if (i < 2)
                    Logger::InfoLog(LOC, "d=" + ToString(d) +
                                             " total_data_sent=" + ToString(chl.getTotalDataSent()) + " bytes");
                chl.resetStats();
            }
            timer_mgr.PrintAllResults("Eval d=" + ToString(d), ringoa::MICROSECONDS, true);
        };

        net_mgr.AutoConfigure(party_id, server_task, client_task);
        net_mgr.WaitForCompletion();
    }
    Logger::InfoLog(LOC, "DpfPir_Online_Bench - Finished");
}

}    // namespace bench_ringoa
