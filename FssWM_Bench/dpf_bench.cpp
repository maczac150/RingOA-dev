#include "dpf_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/fss/dpf_eval.h"
#include "FssWM/fss/dpf_gen.h"
#include "FssWM/fss/dpf_key.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"

namespace bench_fsswm {

using fsswm::block;
using fsswm::FileIo;
using fsswm::GlobalRng;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::ToString;
using fsswm::TimerManager;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::fss::dpf::EvalType;

void Dpf_Fde_Bench() {
    uint64_t              repeat     = 20;
    std::vector<uint64_t> sizes      = {12, 16, 20, 24, 26};
    std::vector<uint64_t> sizes_all  = fsswm::CreateSequence(9, 30);
    std::vector<EvalType> eval_types = {
        EvalType::kIterSingleBatch,
    };

    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, size, eval_type);
            uint64_t        n  = params.GetInputBitsize();
            uint64_t        e  = params.GetOutputBitsize();
            uint64_t        nu = params.GetTerminateBitsize();
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
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                // std::vector<uint64_t> outputs_0(1U << size);
                std::vector<block> outputs_0(1U << nu);
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
            }
            timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    // FileIo io(".log");
    // io.WriteToFile("./data/log/dpf_bench", Logger::GetLogList());
}

void Dpf_Fde_One_Bench() {
    uint64_t              repeat     = 20;
    std::vector<uint64_t> sizes      = {12, 16, 20, 24, 26};
    std::vector<uint64_t> sizes_all  = fsswm::CreateSequence(10, 30);
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
            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::vector<block> outputs_0;
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + ToString(size) + " (" + ToString(i) + ")");
            }
            timer_mgr.PrintCurrentResults("n=" + ToString(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
}

}    // namespace bench_fsswm
