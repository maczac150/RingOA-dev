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

std::vector<uint64_t> db_bitsizes = {16, 18, 20, 22, 24, 26, 28};
// std::vector<uint64_t> db_bitsizes = ringoa::CreateSequence(10, 31);

constexpr uint64_t kIterDefault = 10;

}    // namespace

namespace bench_ringoa {

using ringoa::block;
using ringoa::FileIo;
using ringoa::FormatType;
using ringoa::GlobalRng;
using ringoa::Logger;
using ringoa::Mod;
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
    Logger::InfoLog(LOC, "DpfPir_Offline_Bench...");
    uint64_t iter = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;

    for (auto db_bitsize : db_bitsizes) {
        DpfPirParameters params(db_bitsize);
        params.PrintParameters();
        uint64_t           d = params.GetParameters().GetInputBitsize();
        AdditiveSharing2P  ass(d);
        DpfPirKeyGenerator gen(params, ass);
        FileIo             file_io;
        KeyIo              key_io;

        TimerManager timer_mgr;
        int32_t      timer_keygen = timer_mgr.CreateNewTimer("DpfPir KeyGen");
        int32_t      timer_off    = timer_mgr.CreateNewTimer("DpfPir OfflineSetUp");

        std::string key_path = kBenchPirPath + "dpfpirkey_d" + ToString(d);
        std::string db_path  = kBenchPirPath + "db_d" + ToString(d);
        std::string idx_path = kBenchPirPath + "idx_d" + ToString(d);

        for (uint64_t i = 0; i < iter; ++i) {
            timer_mgr.SelectTimer(timer_keygen);
            timer_mgr.Start();
            // Generate keys
            std::pair<DpfPirKey, DpfPirKey> keys = gen.GenerateKeys();
            timer_mgr.Stop("KeyGen(" + ToString(i) + ") d=" + ToString(d));
            // Save keys
            key_io.SaveKey(key_path + "_0", keys.first);
            key_io.SaveKey(key_path + "_1", keys.second);
        }

        timer_mgr.SelectTimer(timer_off);
        timer_mgr.Start();
        // Offline setup
        gen.OfflineSetUp(iter, kBenchPirPath);
        timer_mgr.Stop("OfflineSetUp(0) d=" + ToString(d));
        timer_mgr.PrintAllResults("Gen d=" + ToString(d), ringoa::MICROSECONDS, true);

        // Generate the database and index
        int32_t timer_data = timer_mgr.CreateNewTimer("DpfPir DataGen");
        timer_mgr.SelectTimer(timer_data);
        timer_mgr.Start();
        std::vector<uint64_t> database(1U << d);
        for (size_t i = 0; i < database.size(); ++i) {
            database[i] = i;
        }
        uint64_t index = ass.GenerateRandomValue();
        timer_mgr.Mark("DataGen d=" + ToString(d));

        std::pair<uint64_t, uint64_t> idx_sh = ass.Share(index);
        timer_mgr.Mark("ShareGen d=" + ToString(d));

        // Save data
        file_io.WriteBinary(idx_path + "_0", idx_sh.first);
        file_io.WriteBinary(idx_path + "_1", idx_sh.second);
        file_io.WriteBinary(db_path, database);

        timer_mgr.Mark("ShareSave d=" + ToString(d));
        timer_mgr.PrintCurrentResults("DataGen d=" + ToString(d), ringoa::MILLISECONDS, true);
    }
    Logger::InfoLog(LOC, "DpfPir_Offline_Bench - Finished");
}

void DpfPir_Online_Bench(const osuCrypto::CLP &cmd) {
    Logger::InfoLog(LOC, "DpfPir_Online_Bench...");
    uint64_t    iter     = cmd.isSet("iter") ? cmd.get<uint64_t>("iter") : kIterDefault;
    int         party_id = cmd.isSet("party") ? cmd.get<int>("party") : -1;
    std::string network  = cmd.isSet("network") ? cmd.get<std::string>("network") : "";

    for (auto db_bitsize : db_bitsizes) {
        DpfPirParameters params(db_bitsize);
        // params.PrintDpfPirParameters();
        uint64_t d  = params.GetDatabaseSize();
        uint64_t nu = params.GetParameters().GetTerminateBitsize();
        KeyIo    key_io;
        FileIo   file_io;

        // Start network communication
        TwoPartyNetworkManager net_mgr("DpfPir_Online_Bench");

        std::string key_path = kBenchPirPath + "dpfpirkey_d" + ToString(d);
        std::string idx_path = kBenchPirPath + "idx_d" + ToString(d);
        std::string db_path  = kBenchPirPath + "db_d" + ToString(d);

        uint64_t              idx_0, idx_1;
        uint64_t              y_0, y_1;
        std::vector<uint64_t> database;
        file_io.ReadBinary(db_path, database);

        // Server task
        auto server_task = [&](oc::Channel &chl) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("DpfPir SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("DpfPir Eval");

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
            timer_mgr.Stop("SetUp(0) d=" + ToString(d));

            timer_mgr.SelectTimer(timer_eval);

            for (uint64_t i = 0; i < iter; ++i) {
                timer_mgr.Start();
                // Evaluate
                y_0 = eval.Evaluate(chl, key_0, uv, database, idx_0);
                timer_mgr.Stop("Evaluate(" + ToString(i) + ") d=" + ToString(d));

                if (i < 2)
                    Logger::InfoLog(LOC, "Total data sent: " + ToString(chl.getTotalDataSent()) + " bytes");
                chl.resetStats();
            }
            timer_mgr.PrintAllResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        };

        // Client task
        auto client_task = [&](oc::Channel &chl) {
            TimerManager timer_mgr;
            int32_t      timer_setup = timer_mgr.CreateNewTimer("DpfPir SetUp");
            int32_t      timer_eval  = timer_mgr.CreateNewTimer("DpfPir Eval");

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
            eval.OnlineSetUp(1, kBenchPirPath);
            timer_mgr.Stop("SetUp(0) d=" + ToString(d));

            timer_mgr.SelectTimer(timer_eval);

            for (uint64_t i = 0; i < iter; ++i) {
                timer_mgr.Start();
                // Evaluate
                y_1 = eval.Evaluate(chl, key_1, uv, database, idx_1);
                timer_mgr.Stop("Evaluate(" + ToString(i) + ") d=" + ToString(d));

                if (i < 2)
                    Logger::InfoLog(LOC, "Total data sent: " + ToString(chl.getTotalDataSent()) + " bytes");
                chl.resetStats();
            }
            timer_mgr.PrintAllResults("d=" + ToString(d), ringoa::MICROSECONDS, true);
        };

        net_mgr.AutoConfigure(party_id, server_task, client_task);
        net_mgr.WaitForCompletion();
    }
    Logger::InfoLog(LOC, "DpfPir_Online_Bench - Finished");
}

}    // namespace bench_ringoa
