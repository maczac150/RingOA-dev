#include "dpf_bench.h"

#include <cryptoTools/Common/TestCollection.h>

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"
#include "RingOA/utils/file_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"
#include "bench_common.h"

namespace bench_ringoa {

using ringoa::block;
using ringoa::GlobalRng;
using ringoa::Logger;
using ringoa::Mod2N;
using ringoa::TimerManager;
using ringoa::ToString;
using ringoa::fss::EvalType, ringoa::fss::OutputType;
using ringoa::fss::dpf::DpfEvaluator;
using ringoa::fss::dpf::DpfKey;
using ringoa::fss::dpf::DpfKeyGenerator;
using ringoa::fss::dpf::DpfParameters;

void Dpf_Fde_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat     = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> sizes      = SelectBitsizes(cmd);
    std::vector<EvalType> eval_types = {
        EvalType::kHybridBatched,
    };

    Logger::InfoLog(LOC, "FDE Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, size, eval_type);
            uint64_t        n  = params.GetInputBitsize();
            uint64_t        e  = params.GetOutputBitsize();
            uint64_t        nu = params.GetTerminateBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);

            uint64_t alpha = Mod2N(GlobalRng::Rand<uint64_t>(), n);
            uint64_t beta  = Mod2N(GlobalRng::Rand<uint64_t>(), e);

            std::vector<block> outputs_0(1ULL << nu);
            std::vector<block> outputs_1(1ULL << nu);

            const std::string timer_name = "DPF-FDE Eval (P0)";

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop(
                    "n=" + ToString(n) +
                    " e=" + ToString(e) +
                    " eval=" + GetEvalTypeString(params.GetEvalType()) +
                    " iter=" + ToString(i));

                eval.EvaluateFullDomain(keys.second, outputs_1);
            }

            const std::string summary_msg =
                "n=" + ToString(n) +
                " e=" + ToString(e) +
                " eval=" + GetEvalTypeString(params.GetEvalType());

            timer_mgr.PrintCurrentResults(
                summary_msg,
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    Logger::ExportLogListAndClear(kLogDpfPath + "dpf_fde_bench", /*use_timestamp=*/true);
}

void Dpf_Fde_Convert_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat     = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> sizes      = SelectBitsizes(cmd);
    std::vector<EvalType> eval_types = {
        EvalType::kHybridBatched,
        EvalType::kIterative,
    };

    Logger::InfoLog(LOC, "FDE Convert Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, size, eval_type);
            uint64_t        n = params.GetInputBitsize();
            uint64_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);

            uint64_t alpha = Mod2N(GlobalRng::Rand<uint64_t>(), n);
            uint64_t beta  = Mod2N(GlobalRng::Rand<uint64_t>(), e);

            std::vector<uint64_t> outputs_0(1U << size);
            std::vector<uint64_t> outputs_1(1U << size);

            const std::string timer_name = "DPF-FDE-Convert Eval P0";

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop(
                    "n=" + ToString(n) +
                    " e=" + ToString(e) +
                    " eval=" + GetEvalTypeString(params.GetEvalType()) +
                    " iter=" + ToString(i));

                eval.EvaluateFullDomain(keys.second, outputs_1);
            }

            const std::string summary_msg =
                "n=" + ToString(n) +
                " e=" + ToString(e) +
                " eval=" + GetEvalTypeString(params.GetEvalType());

            timer_mgr.PrintCurrentResults(
                summary_msg,
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    Logger::ExportLogListAndClear(kLogDpfPath + "dpf_fde_conv_bench", /*use_timestamp=*/true);
}

void Dpf_Fde_One_Bench(const osuCrypto::CLP &cmd) {
    uint64_t              repeat     = cmd.getOr("repeat", kRepeatDefault);
    std::vector<uint64_t> sizes      = SelectBitsizes(cmd);
    std::vector<EvalType> eval_types = {
        EvalType::kHybridBatched,
    };

    Logger::InfoLog(LOC, "FDE One Benchmark started (repeat=" + ToString(repeat) + ")");

    for (auto eval_type : eval_types) {
        for (auto size : sizes) {
            DpfParameters   params(size, 1, eval_type);
            uint64_t        n  = params.GetInputBitsize();
            uint64_t        e  = params.GetOutputBitsize();
            uint64_t        nu = params.GetTerminateBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);

            uint64_t alpha = Mod2N(GlobalRng::Rand<uint64_t>(), n);
            uint64_t beta  = 1;

            std::vector<block> outputs_0(1ULL << nu);
            std::vector<block> outputs_1(1ULL << nu);

            const std::string timer_name = "DPF-FDE-One Eval P0";

            TimerManager timer_mgr;
            int32_t      timer_id = timer_mgr.CreateNewTimer(timer_name);
            timer_mgr.SelectTimer(timer_id);

            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

            for (uint64_t i = 0; i < repeat; ++i) {
                timer_mgr.Start();
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop(
                    "n=" + ToString(n) +
                    " e=" + ToString(e) +
                    " eval=" + GetEvalTypeString(params.GetEvalType()) +
                    " iter=" + ToString(i));

                eval.EvaluateFullDomain(keys.second, outputs_1);
            }

            const std::string summary_msg =
                "n=" + ToString(n) +
                " e=" + ToString(e) +
                " eval=" + GetEvalTypeString(params.GetEvalType());

            timer_mgr.PrintCurrentResults(
                summary_msg,
                ringoa::TimeUnit::MICROSECONDS,
                /*show_details=*/true);
        }
    }
    Logger::InfoLog(LOC, "FDE Benchmark completed");
    Logger::ExportLogListAndClear(kLogDpfPath + "dpf_fde_one_bench", /*use_timestamp=*/true);
}

}    // namespace bench_ringoa
