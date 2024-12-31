#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/fss/dpf_eval.hpp"
#include "FssWM/fss/dpf_gen.hpp"
#include "FssWM/fss/dpf_key.hpp"
#include "FssWM/utils/logger.hpp"
#include "FssWM/utils/rng.hpp"
#include "FssWM/utils/timer.hpp"
#include "FssWM/utils/utils.hpp"

namespace {

bool DpfFullDomainCheck(const uint32_t alpha, const uint32_t beta, const std::vector<uint32_t> &res) {
    bool check = true;
    for (uint32_t i = 0; i < res.size(); i++) {
        if ((i == alpha && res[i] == beta) || (i != alpha && res[i] == 0)) {
            check &= true;
        } else {
            check &= false;
            utils::Logger::DebugLog(LOC, "FDE check failed at x=" + std::to_string(i) + " -> Result: " + std::to_string(res[i]), true);
        }
    }
    return check;
}

// Check if the XOR sum of all blocks matches the expected block (Note: Can detect only error exists, not the position)
bool DpfFullDomainCheckOneBit(const uint32_t alpha, const uint32_t beta, const std::vector<block> &res) {
    // Compute XOR sum of all blocks
    block xor_sum = zero_block;
    for (const auto &r : res) {
        xor_sum = xor_sum ^ r;
    }
    // Calculate bit position
    uint32_t bit_position = alpha % (sizeof(block) * 8);
    // Generate block with only the bit_position set to 1
    uint64_t high = 0, low = 0;
    // Set the bit_position to 1
    if (bit_position < 64) {
        low = 1ULL << bit_position;
    } else {
        high = 1ULL << (bit_position - 64);
    }
    block expected_block = makeBlock(high, low);
    // Check if XOR sum matches the expected block
    bool is_match = fsswm::fss::Equal(xor_sum, expected_block);
    if (!is_match) {
        utils::Logger::DebugLog(LOC, "FDE check failed for alpha=" + std::to_string(alpha) + " and beta=" + std::to_string(beta), true);
    }
    return is_match;
}

}    // namespace

namespace test_fsswm {

using namespace fsswm::fss;

void Dpf_Params_Test() {
    std::vector<dpf::DpfParameters> params_list = {
        dpf::DpfParameters(5, 5, false),
        dpf::DpfParameters(5, 5, false),
        dpf::DpfParameters(5, 1, false),
        dpf::DpfParameters(5, 1, true),
        dpf::DpfParameters(10, 10, false),
        dpf::DpfParameters(10, 10, true),
        dpf::DpfParameters(10, 1, false),
        dpf::DpfParameters(10, 1, true),
        dpf::DpfParameters(20, 20, false),
        dpf::DpfParameters(20, 20, true),
        dpf::DpfParameters(20, 1, false),
        dpf::DpfParameters(20, 1, true),
    };

    for (const dpf::DpfParameters &params : params_list) {
        // params.PrintDpfParameters();
        uint32_t                            e = params.GetElementBitsize();
        dpf::DpfKeyGenerator                gen(params);
        dpf::DpfEvaluator                   eval(params);
        uint32_t                            alpha = 5;
        uint32_t                            beta  = 1;
        std::pair<dpf::DpfKey, dpf::DpfKey> keys  = gen.GenerateKeys(alpha, beta);

        uint32_t x   = 5;
        uint32_t y_0 = eval.EvaluateAt(keys.first, x);
        uint32_t y_1 = eval.EvaluateAt(keys.second, x);
        uint32_t y   = utils::Mod(y_0 + y_1, e);

        if (y != beta)
            throw oc::UnitTestFail("y is not equal to beta");

        x   = 10;
        y_0 = eval.EvaluateAt(keys.first, x);
        y_1 = eval.EvaluateAt(keys.second, x);
        y   = utils::Mod(y_0 + y_1, e);

        if (y != 0)
            throw oc::UnitTestFail("y is not equal to 0");
    }
}

void Dpf_Fde_Type_Test() {
    for (auto eval_type : {
             dpf::EvalType::kNaive,
             dpf::EvalType::kRecursion,
             dpf::EvalType::kIterSingle,
             dpf::EvalType::kIterDouble,
             dpf::EvalType::kIterSingleBatch,
             dpf::EvalType::kIterDoubleBatch,
         }) {

        utils::Logger::DebugLog(LOC, "Evaluation type: " + dpf::GetEvalTypeString(eval_type), true);

        std::vector<dpf::DpfParameters> params_list = {
            dpf::DpfParameters(5, 5, false, eval_type),
            dpf::DpfParameters(5, 1, false, eval_type),
            dpf::DpfParameters(5, 5, true, eval_type),
            dpf::DpfParameters(5, 1, true, eval_type),
            dpf::DpfParameters(10, 10, false, eval_type),
            dpf::DpfParameters(10, 1, false, eval_type),
            dpf::DpfParameters(10, 10, true, eval_type),
            dpf::DpfParameters(10, 1, true, eval_type),
            dpf::DpfParameters(11, 11, false, eval_type),
            dpf::DpfParameters(11, 1, false, eval_type),
            dpf::DpfParameters(11, 11, true, eval_type),
            dpf::DpfParameters(11, 1, true, eval_type),
            dpf::DpfParameters(20, 20, false, eval_type),
            dpf::DpfParameters(20, 1, false, eval_type),
            dpf::DpfParameters(20, 20, true, eval_type),
            dpf::DpfParameters(20, 1, true, eval_type),
        };

        for (const dpf::DpfParameters &params : params_list) {
            // params.PrintDpfParameters();
            dpf::DpfKeyGenerator gen(params);
            dpf::DpfEvaluator    eval(params);
        }
    }
}

void Dpf_Fde_Test() {
    for (auto eval_type : {
             dpf::EvalType::kNaive,
             dpf::EvalType::kRecursion,
             dpf::EvalType::kIterSingle,
             dpf::EvalType::kIterDouble,
             dpf::EvalType::kIterSingleBatch,
             dpf::EvalType::kIterDoubleBatch,
         }) {

        std::vector<dpf::DpfParameters> params_list = {
            dpf::DpfParameters(5, 5, true, eval_type),
            dpf::DpfParameters(10, 10, true, eval_type),
            dpf::DpfParameters(15, 15, true, eval_type),
            dpf::DpfParameters(20, 20, true, eval_type),
        };

        for (const dpf::DpfParameters &params : params_list) {
            // params.PrintDpfParameters();
            uint32_t             n = params.GetInputBitsize();
            uint32_t             e = params.GetElementBitsize();
            dpf::DpfKeyGenerator gen(params);
            dpf::DpfEvaluator    eval(params);
            uint32_t             alpha = utils::Mod(utils::SecureRng::Rand64(), n);
            uint32_t             beta  = utils::Mod(utils::SecureRng::Rand64(), e);

            // Generate keys
            utils::Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta), true);
            std::pair<dpf::DpfKey, dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys
            std::vector<uint32_t> outputs_0, outputs_1;
            eval.EvaluateFullDomain(keys.first, outputs_0);
            eval.EvaluateFullDomain(keys.second, outputs_1);

            std::vector<uint32_t> outputs(outputs_0.size());
            for (uint32_t i = 0; i < outputs_0.size(); i++) {
                outputs[i] = utils::Mod(outputs_0[i] + outputs_1[i], e);
            }
#ifdef LOG_LEVEL_DEBUG
            utils::Logger::DebugLog(LOC, "Outputs=" + utils::ToString(outputs), true);
#endif

            // Check FDE
            if (!DpfFullDomainCheck(alpha, beta, outputs))
                throw oc::UnitTestFail("FDE check failed");
        }
    }
}

void Dpf_Fde_One_Test() {
    for (auto eval_type : {
             //  dpf::EvalType::kNaive, // * Not supported
             dpf::EvalType::kRecursion,
             dpf::EvalType::kIterSingle,
             dpf::EvalType::kIterDouble,
             dpf::EvalType::kIterSingleBatch,
             dpf::EvalType::kIterDoubleBatch,
         }) {

        std::vector<dpf::DpfParameters> params_list = {
            // dpf::DpfParameters(5, 1, false), // * Not supported
            dpf::DpfParameters(11, 1, true, eval_type),
            dpf::DpfParameters(15, 1, true, eval_type),
            dpf::DpfParameters(20, 1, true, eval_type),
            dpf::DpfParameters(25, 1, true, eval_type),
        };

        for (const dpf::DpfParameters &params : params_list) {
            // params.PrintDpfParameters();
            uint32_t             n = params.GetInputBitsize();
            dpf::DpfKeyGenerator gen(params);
            dpf::DpfEvaluator    eval(params);
            uint32_t             alpha = utils::Mod(utils::SecureRng::Rand64(), n);
            uint32_t             beta  = 1;

            // Generate keys
            utils::Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta), true);
            std::pair<dpf::DpfKey, dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys
            std::vector<block> outputs_0, outputs_1;
            eval.EvaluateFullDomainOneBit(keys.first, outputs_0);
            eval.EvaluateFullDomainOneBit(keys.second, outputs_1);
            std::vector<block> outputs(outputs_0.size());
            for (uint32_t i = 0; i < outputs_0.size(); i++) {
                outputs[i] = outputs_0[i] ^ outputs_1[i];
            }

#ifdef LOG_LEVEL_DEBUG
            for (uint32_t i = 0; i < outputs.size(); i++) {
                utils::Logger::InfoLog(LOC, "Outputs[" + std::to_string(i) + "]=" + ToString(outputs[i], FormatType::kBin));
            }
#endif

            // Check FDE
            if (!DpfFullDomainCheckOneBit(alpha, beta, outputs))
                throw oc::UnitTestFail("FDE check failed");
        }
    }
}

void Dpf_Fde_Bench_Test() {
    uint32_t                   repeat     = 50;
    std::vector<uint32_t>      sizes      = {16, 20, 24, 27};
    std::vector<uint32_t>      sizes_all  = utils::CreateSequence(9, 30);
    std::vector<dpf::EvalType> eval_types = {
        dpf::EvalType::kRecursion,
        dpf::EvalType::kIterSingle,
        dpf::EvalType::kIterDouble,
        dpf::EvalType::kIterSingleBatch,
        dpf::EvalType::kIterDoubleBatch,
    };

    utils::Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : eval_types) {
        for (auto size : sizes_all) {
            // for (auto eval_type : eval_types) {
            //     for (auto size : sizes) {
            dpf::DpfParameters   params(size, size, true, eval_type);
            uint32_t             n = params.GetInputBitsize();
            uint32_t             e = params.GetElementBitsize();
            dpf::DpfKeyGenerator gen(params);
            dpf::DpfEvaluator    eval(params);
            uint32_t             alpha = utils::Mod(utils::SecureRng::Rand64(), n);
            uint32_t             beta  = utils::Mod(utils::SecureRng::Rand64(), e);

            utils::TimerManager timer_mgr;
            int32_t             timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + dpf::GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<dpf::DpfKey, dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);
            // Evaluate keys
            for (uint32_t i = 0; i < repeat; i++) {
                timer_mgr.Start();
                std::vector<uint32_t> outputs_0;
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
            }
            timer_mgr.PrintCurrentResults("n=" + std::to_string(size), utils::TimeUnit::MICROSECONDS, true);
        }
    }
    utils::Logger::InfoLog(LOC, "FDE Benchmark completed");
    utils::Logger::SaveLogsToFile("./log/fde_bench");
}

void Dpf_Fde_One_Bench_Test() {
    uint32_t repeat = 50;
    utils::Logger::InfoLog(LOC, "FDE Benchmark started");
    for (auto eval_type : {
             dpf::EvalType::kRecursion,
             dpf::EvalType::kIterSingle,
             dpf::EvalType::kIterDouble,
             dpf::EvalType::kIterSingleBatch,
             dpf::EvalType::kIterDoubleBatch,
         }) {

        for (uint32_t size = 11; size <= 29; size += 1) {
            dpf::DpfParameters   params(size, 1, true, eval_type);
            uint32_t             n = params.GetInputBitsize();
            dpf::DpfKeyGenerator gen(params);
            dpf::DpfEvaluator    eval(params);
            uint32_t             alpha = utils::Mod(utils::SecureRng::Rand64(), n);
            uint32_t             beta  = 1;

            utils::TimerManager timer_mgr;
            int32_t             timer_id = timer_mgr.CreateNewTimer("FDE Benchmark:" + dpf::GetEvalTypeString(params.GetFdeEvalType()));
            timer_mgr.SelectTimer(timer_id);

            // Generate keys
            std::pair<dpf::DpfKey, dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys
            for (uint32_t i = 0; i < repeat; i++) {
                timer_mgr.Start();
                std::vector<block> outputs_0;
                eval.EvaluateFullDomainOneBit(keys.first, outputs_0);
                timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
            }
            timer_mgr.PrintCurrentResults("n=" + std::to_string(size), utils::TimeUnit::MICROSECONDS, true);
        }
    }
    utils::Logger::InfoLog(LOC, "FDE Benchmark completed");
    // utils::Logger::SaveLogsToFile("./log/fde_one_bench");
}

}    // namespace test_fsswm
