#include "dpf_bench.h"

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

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
using fsswm::Logger;
using fsswm::Mod;
using fsswm::SecureRng;
using fsswm::TimerManager;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::fss::dpf::EvalType;

void Dpf_Fde_Bench() {
    uint32_t              repeat     = 500;
    std::vector<uint32_t> sizes      = {16, 20, 24, 27};
    std::vector<uint32_t> sizes_all  = fsswm::CreateSequence(9, 30);
    std::vector<EvalType> eval_types = {
        // EvalType::kNaive,
        // EvalType::kRecursion,
        EvalType::kIterSingle,
        EvalType::kIterDouble,
        EvalType::kIterSingleBatch,
        EvalType::kIterDoubleBatch,
    };

    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : eval_types) {
        for (auto size : sizes_all) {
            // for (auto eval_type : eval_types) {
            //     for (auto size : sizes) {
            DpfParameters   params(size, size, true, eval_type);
            uint32_t        n = params.GetInputBitsize();
            uint32_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint32_t        alpha = Mod(SecureRng::Rand32(), n);
            uint32_t        beta  = Mod(SecureRng::Rand32(), e);

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);
            // Evaluate keys
            for (uint32_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::vector<uint32_t> outputs_0(1U << size);
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
            }
            timer_mgr.PrintCurrentResults("n=" + std::to_string(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    FileIo io(".log");
    io.WriteToFile("./data/log/dpf_bench", Logger::GetLogList());
}

void Dpf_Fde_One_Bench() {
    uint32_t repeat = 50;
    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : {
             EvalType::kRecursion,
             EvalType::kIterSingle,
             EvalType::kIterDouble,
             EvalType::kIterSingleBatch,
             EvalType::kIterDoubleBatch,
         }) {

        for (uint32_t size = 11; size <= 29; size += 1) {
            DpfParameters   params(size, 1, true, eval_type);
            uint32_t        n = params.GetInputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint32_t        alpha = Mod(SecureRng::Rand32(), n);
            uint32_t        beta  = 1;

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys
            for (uint32_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                std::vector<block> outputs_0;
                eval.EvaluateFullDomainOneBit(keys.first, outputs_0);
                timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
            }
            timer_mgr.PrintCurrentResults("n=" + std::to_string(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
}

void Dpf_Fde_TwoKey_Bench() {
    uint32_t              repeat    = 10;
    std::vector<uint32_t> sizes     = {16, 20, 24, 27};
    std::vector<uint32_t> sizes_all = fsswm::CreateSequence(10, 29);

    Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : {
             EvalType::kIterSingleBatch,
             EvalType::kIterSingleBatch_2Keys,
         }) {
        for (auto size : sizes_all) {
            DpfParameters   params(size, size, true, eval_type);
            uint32_t        n = params.GetInputBitsize();
            uint32_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint32_t        alpha1 = Mod(SecureRng::Rand32(), n);
            uint32_t        beta1  = Mod(SecureRng::Rand32(), e);
            uint32_t        alpha2 = Mod(SecureRng::Rand32(), n);
            uint32_t        beta2  = Mod(SecureRng::Rand32(), e);

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<DpfKey, DpfKey> keys1 = gen.GenerateKeys(alpha1, beta1);
            std::pair<DpfKey, DpfKey> keys2 = gen.GenerateKeys(alpha2, beta2);
            // Evaluate keys
            for (uint32_t i = 0; i < repeat; ++i) {
                if (eval_type == EvalType::kIterSingleBatch) {
                    timer_mgr.Start();
                    std::vector<uint32_t> out0_0, out0_1;
                    eval.EvaluateFullDomain(keys1.first, out0_0);
                    eval.EvaluateFullDomain(keys1.second, out0_1);
                    timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
                } else if (eval_type == EvalType::kIterSingleBatch_2Keys) {
                    timer_mgr.Start();
                    std::vector<uint32_t> out0_0, out0_1;
                    eval.EvaluateFullDomainTwoKeys(keys1.first, keys2.first, out0_0, out0_1);
                    timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
                }
            }
            timer_mgr.PrintCurrentResults("n=" + std::to_string(size), fsswm::TimeUnit::MICROSECONDS, true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
}

}    // namespace bench_fsswm
