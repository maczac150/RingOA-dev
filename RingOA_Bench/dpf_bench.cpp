#include "dpf_bench.h"

#include <cryptoTools/Common/TestCollection.h>
#include <thread>

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"
#include "RingOA/utils/file_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace bench_ringoa {

using ringoa::block;
using ringoa::FileIo;
using ringoa::GlobalRng;
using ringoa::Logger;
using ringoa::Mod;
using ringoa::TimerManager;
using ringoa::ToString;
using ringoa::fss::EvalType, ringoa::fss::OutputType;
using ringoa::fss::dpf::DpfEvaluator;
using ringoa::fss::dpf::DpfKey;
using ringoa::fss::dpf::DpfKeyGenerator;
using ringoa::fss::dpf::DpfParameters;

void Dpf_Fde_Bench() {
    uint64_t              repeat = 10;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = ringoa::CreateSequence(10, 30);
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
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetEvalType()));
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
            timer_mgr.PrintCurrentResults("n=" + ToString(size), ringoa::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    Logger::ExportLogList("./data/logs/dpf_fde_bench");
}

void Dpf_Fde_Convert_Bench() {
    uint64_t              repeat = 10;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = ringoa::CreateSequence(10, 30);
    std::vector<EvalType> eval_types = {
        EvalType::kIterSingleBatch,
        EvalType::kIterDepthFirst,
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
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetEvalType()));
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
            timer_mgr.PrintCurrentResults("n=" + ToString(size), ringoa::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    Logger::ExportLogList("./data/logs/dpf_fde_conv_bench");
}

void Dpf_Fde_One_Bench() {
    uint64_t              repeat = 50;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = ringoa::CreateSequence(10, 30);
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
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetEvalType()));
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
            timer_mgr.PrintCurrentResults("n=" + ToString(size), ringoa::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    Logger::ExportLogList("./data/logs/dpf_fde_one_bench");
}

}    // namespace bench_ringoa
