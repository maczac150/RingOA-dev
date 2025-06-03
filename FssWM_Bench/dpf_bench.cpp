#include "dpf_bench.h"

#include <cryptoTools/Common/TestCollection.h>
#include <thread>

#include "FssWM/fss/dpf_eval.h"
#include "FssWM/fss/dpf_gen.h"
#include "FssWM/fss/dpf_key.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace bench_fsswm {

using fsswm::block;
using fsswm::FileIo;
using fsswm::GlobalRng;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::fss::EvalType, fsswm::fss::OutputMode;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;

void Dpf_Fde_Bench() {
    uint64_t              repeat = 10;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = fsswm::CreateSequence(10, 30);
    std::vector<EvalType> eval_types = {
        EvalType::kIterSingleBatch,
    };

    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, size, eval_type);
            uint64_t        n = params.GetInputBitsize();
            uint64_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
            uint64_t        beta  = Mod(GlobalRng::Rand<uint64_t>(), e);

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);
            // Evaluate keys
            std::vector<block> outputs_0(1U << params.GetTerminateBitsize()), outputs_1(1U << params.GetTerminateBitsize());
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
                eval.EvaluateFullDomain(keys.second, outputs_1);
            }
            timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    // Logger::ExportLogList("./data/log/dpf_bench");
}

void Dpf_Fde_Convert_Bench() {
    uint64_t              repeat = 10;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = fsswm::CreateSequence(10, 30);
    std::vector<EvalType> eval_types = {
        EvalType::kIterSingleBatch,
    };

    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, size, eval_type);
            uint64_t        n = params.GetInputBitsize();
            uint64_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
            uint64_t        beta  = Mod(GlobalRng::Rand<uint64_t>(), e);

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);
            // Evaluate keys
            std::vector<uint64_t> outputs_0(1U << size), outputs_1(1U << size);
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
                eval.EvaluateFullDomain(keys.second, outputs_1);
            }
            timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    // Logger::ExportLogList("./data/log/dpf_bench");
}

void Dpf_Fde_One_Bench() {
    uint64_t              repeat = 50;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = fsswm::CreateSequence(10, 30);
    std::vector<EvalType> eval_types = {
        EvalType::kIterSingleBatch,
    };

    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, 1, eval_type);
            uint64_t        n = params.GetInputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
            uint64_t        beta  = 1;

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys
            std::vector<block> outputs_0(1U << params.GetTerminateBitsize()), outputs_1(1U << params.GetTerminateBitsize());
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
                eval.EvaluateFullDomain(keys.second, outputs_1);
            }
            timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
}

void Dpf_Pir_Bench() {
    uint64_t              repeat = 50;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = fsswm::CreateSequence(10, 30);

    Logger::InfoLog(LOC, "Pir Benchmark started");
    for (auto size : sizes) {
        DpfParameters   params(size, 1, EvalType::kIterSingleBatch);
        uint64_t        n = params.GetInputBitsize();
        DpfKeyGenerator gen(params);
        DpfEvaluator    eval(params);
        uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
        uint64_t        beta  = 1;

        TimerManager timer_mgr;
        int32_t      timer_id = timer_mgr.CreateNewTimer("Pir Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
        timer_mgr.SelectTimer(timer_id);

        // Generate keys
        std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);
        std::vector<block>        database(1 << n);
        for (uint64_t i = 0; i < database.size(); ++i) {
            database[i] = fsswm::MakeBlock(0, i);
        }

        // Evaluate keys
        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            block result_0 = eval.EvaluatePir(keys.first, database);
            timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
            // Use result_0 in the check below to prevent the compiler from
            // optimizing away the PIR evaluation call. If result_0 were never used,
            // the compiler might inline or remove the call entirely, invalidating
            // the timing measurement.
            block result_1 = eval.EvaluatePir(keys.second, database);
            if ((result_0 ^ result_1) != database[alpha]) {
                Logger::FatalLog(LOC, "Pir evaluation failed: result_0=" + fsswm::Format(result_0) + ", result_1=" + fsswm::Format(result_1) + ", expected=" + fsswm::Format(database[alpha]));
                std::exit(EXIT_FAILURE);
            }
        }
        timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
    }
    Logger::InfoLog(LOC, "Pir Benchmark completed");
}

void Dpf_Pir_Shift_Bench() {
    uint64_t              repeat = 50;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = fsswm::CreateSequence(10, 30);

    Logger::InfoLog(LOC, "Pir Shift Benchmark started");
    for (auto size : sizes) {
        DpfParameters   params(size, 1, EvalType::kIterSingleBatch, OutputMode::kAdditive);
        uint64_t        n = params.GetInputBitsize();
        DpfKeyGenerator gen(params);
        DpfEvaluator    eval(params);
        uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
        uint64_t        beta  = 1;

        TimerManager timer_mgr;
        int32_t      timer_id = timer_mgr.CreateNewTimer("Pir Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
        timer_mgr.SelectTimer(timer_id);

        // Generate keys
        std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);
        std::vector<uint64_t>     database(1 << n);
        for (uint64_t i = 0; i < database.size(); ++i) {
            database[i] = i;
        }

        // Evaluate keys
        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            uint64_t result_0 = eval.EvaluatePir_128bitshift(keys.first, database);
            timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
            // Use result_0 in the check below to prevent the compiler from
            // optimizing away the PIR evaluation call. If result_0 were never used,
            // the compiler might inline or remove the call entirely, invalidating
            // the timing measurement.
            uint64_t result_1 = eval.EvaluatePir_128bitshift(keys.second, database);
            if ((result_0 ^ result_1) != database[alpha]) {
                Logger::FatalLog(LOC, "Pir evaluation failed: result_0=" + fsswm::ToString(result_0) + ", result_1=" + fsswm::ToString(result_1) + ", expected=" + fsswm::ToString(database[alpha]));
                std::exit(EXIT_FAILURE);
            }
        }
        timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
    }
    Logger::InfoLog(LOC, "Pir Benchmark completed");
}

void Dpf_Pir_Then_Bench() {
    uint64_t              repeat = 50;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = fsswm::CreateSequence(10, 30);

    Logger::InfoLog(LOC, "Pir Shift Benchmark started");
    for (auto size : sizes) {
        DpfParameters   params(size, 1, EvalType::kIterSingleBatch, OutputMode::kAdditive);
        uint64_t        n = params.GetInputBitsize();
        DpfKeyGenerator gen(params);
        DpfEvaluator    eval(params);
        uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
        uint64_t        beta  = 1;

        TimerManager timer_mgr;
        int32_t      timer_id = timer_mgr.CreateNewTimer("Pir Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
        timer_mgr.SelectTimer(timer_id);

        // Generate keys
        std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);
        std::vector<uint64_t>     database(1 << n);
        for (uint64_t i = 0; i < database.size(); ++i) {
            database[i] = i;
        }

        // Evaluate keys
        std::vector<block> outputs_0(1U << params.GetTerminateBitsize()), outputs_1(1U << params.GetTerminateBitsize());
        for (uint64_t i = 0; i < repeat; ++i) {
            timer_mgr.Start();
            uint64_t result_0 = eval.EvaluatePir_FdeThenDP(keys.first, outputs_0, database);
            timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
            // Use result_0 in the check below to prevent the compiler from
            // optimizing away the PIR evaluation call. If result_0 were never used,
            // the compiler might inline or remove the call entirely, invalidating
            // the timing measurement.
            uint64_t result_1 = eval.EvaluatePir_FdeThenDP(keys.second, outputs_1, database);
            if ((result_0 ^ result_1) != database[alpha]) {
                Logger::FatalLog(LOC, "Pir evaluation failed: result_0=" + fsswm::ToString(result_0) + ", result_1=" + fsswm::ToString(result_1) + ", expected=" + fsswm::ToString(database[alpha]));
                std::exit(EXIT_FAILURE);
            }
        }
        timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
    }
    Logger::InfoLog(LOC, "Pir Benchmark completed");
}

}    // namespace bench_fsswm
