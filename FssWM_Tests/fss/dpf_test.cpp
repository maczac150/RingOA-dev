#include "dpf_test.h"

#include "cryptoTools/Common/TestCollection.h"

#include "FssWM/fss/dpf_eval.h"
#include "FssWM/fss/dpf_gen.h"
#include "FssWM/fss/dpf_key.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/timer.h"

namespace {

bool DpfFullDomainCheck(const uint32_t alpha, const uint32_t beta, const std::vector<uint32_t> &res) {
    bool check = true;
    for (uint32_t i = 0; i < res.size(); ++i) {
        if ((i == alpha && res[i] == beta) || (i != alpha && res[i] == 0)) {
            check &= true;
        } else {
            check &= false;
            fsswm::Logger::DebugLog(LOC, "FDE check failed at x=" + std::to_string(i) + " -> Result: " + std::to_string(res[i]));
        }
    }
    return check;
}

// Check if the XOR sum of all blocks matches the expected block (Note: Can detect only error exists, not the position)
bool DpfFullDomainCheckOneBit(const uint32_t alpha, const uint32_t beta, const std::vector<fsswm::block> &res) {
    // Compute XOR sum of all blocks
    fsswm::block xor_sum = fsswm::zero_block;
    for (const auto &r : res) {
        xor_sum = xor_sum ^ r;
    }
    // Calculate bit position
    uint32_t bit_position = alpha % (sizeof(fsswm::block) * 8);
    // Generate block with only the bit_position set to 1
    uint64_t high = 0, low = 0;
    // Set the bit_position to 1
    if (bit_position < 64) {
        low = 1ULL << bit_position;
    } else {
        high = 1ULL << (bit_position - 64);
    }
    fsswm::block expected_block = fsswm::makeBlock(high, low);
    // Check if XOR sum matches the expected block
    bool is_match = fsswm::Equal(xor_sum, expected_block);
    if (!is_match) {
        fsswm::Logger::DebugLog(LOC, "FDE check failed for alpha=" + std::to_string(alpha) + " and beta=" + std::to_string(beta));
    }
    return is_match;
}

}    // namespace

namespace test_fsswm {

using fsswm::block;
using fsswm::FormatType;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::SecureRng;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::fss::dpf::EvalType;

void Dpf_Params_Test() {
    Logger::DebugLog(LOC, "Dpf_Params_Test...");

    // Test parameters
    const std::vector<std::pair<uint32_t, uint32_t>> size_pair = {
        {3, 3},
        {3, 1},
        {9, 1},
        {10, 1},
        {8, 8},
        {9, 9},
        {17, 17},
        {29, 29},
    };
    const std::vector<EvalType> evals = {
        EvalType::kNaive,
        EvalType::kRecursion,
        EvalType::kIterSingleBatch};

    // Test all combinations of parameters
    for (auto [n, e] : size_pair) {
        for (auto ev : evals) {
            DpfParameters params(n, e, ev);
            params.PrintParameters();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
        }
    }

    Logger::DebugLog(LOC, "Dpf_Params_Test - Passed");
}

void Dpf_EvalAt_Test() {
    Logger::DebugLog(LOC, "Dpf_EvalAt_Test...");
    // Test parameters
    const std::vector<std::pair<uint32_t, uint32_t>> size_pair = {
        {3, 3},
        {3, 1},
        {9, 1},
        {10, 1},
        {8, 8},
        {9, 9},
        {17, 17},
        {29, 29},
    };
    const std::vector<EvalType> evals = {
        EvalType::kNaive,
        EvalType::kIterSingleBatch};

    // Test all combinations of parameters
    for (auto [n, e] : size_pair) {
        for (auto ev : evals) {
            DpfParameters param(n, e, ev);
            param.PrintParameters();
            uint32_t                  e = param.GetOutputBitsize();
            DpfKeyGenerator           gen(param);
            DpfEvaluator              eval(param);
            uint32_t                  alpha = 5;
            uint32_t                  beta  = 1;
            std::pair<DpfKey, DpfKey> keys  = gen.GenerateKeys(alpha, beta);

            uint32_t x   = 5;
            uint32_t y_0 = eval.EvaluateAt(keys.first, x);
            uint32_t y_1 = eval.EvaluateAt(keys.second, x);
            uint32_t y   = Mod(y_0 + y_1, e);

            if (y != beta)
                throw oc::UnitTestFail("y is not equal to beta");

            x   = 7;
            y_0 = eval.EvaluateAt(keys.first, x);
            y_1 = eval.EvaluateAt(keys.second, x);
            y   = Mod(y_0 + y_1, e);

            if (y != 0)
                throw oc::UnitTestFail("y is not equal to 0");
        }
    }

    Logger::DebugLog(LOC, "Dpf_EvalAt_Test - Passed");
}

void Dpf_Fde_Test() {
    Logger::DebugLog(LOC, "Dpf_Fde_Test...");
    const std::vector<std::tuple<uint32_t, uint32_t, EvalType>> fde_param = {
        {3, 3, EvalType::kNaive},
        {8, 8, EvalType::kRecursion},
        {8, 8, EvalType::kIterSingleBatch},
        {9, 9, EvalType::kRecursion},
        {9, 9, EvalType::kIterSingleBatch},
        {17, 17, EvalType::kRecursion},
        {17, 17, EvalType::kIterSingleBatch},
    };

    // Test all combinations of parameters
    for (auto [n, e, eval_type] : fde_param) {
        DpfParameters param(n, e, eval_type);
        param.PrintParameters();
        DpfKeyGenerator gen(param);
        DpfEvaluator    eval(param);
        uint32_t        alpha = Mod(SecureRng::Rand32(), n);
        uint32_t        beta  = Mod(SecureRng::Rand32(), e);

        // Generate keys
        Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta));
        std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        std::vector<uint32_t> outputs_0, outputs_1;
        eval.EvaluateFullDomain(keys.first, outputs_0);
        eval.EvaluateFullDomain(keys.second, outputs_1);

        std::vector<uint32_t> outputs(outputs_0.size());
        for (uint32_t i = 0; i < outputs_0.size(); ++i) {
            outputs[i] = Mod(outputs_0[i] + outputs_1[i], e);
        }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Outputs=" + ToString(outputs));
#endif

        // Check FDE
        if (!DpfFullDomainCheck(alpha, beta, outputs))
            throw oc::UnitTestFail("FDE check failed");
    }
    Logger::DebugLog(LOC, "Dpf_Fde_Test - Passed");
}

void Dpf_Fde_One_Test() {
    Logger::DebugLog(LOC, "Dpf_Fde_One_Test...");
    const std::vector<std::tuple<uint32_t, uint32_t, EvalType>> fde_param = {
        {3, 1, EvalType::kNaive},
        {9, 1, EvalType::kRecursion},
        {9, 1, EvalType::kIterSingleBatch},
        {10, 1, EvalType::kRecursion},
        {10, 1, EvalType::kIterSingleBatch},
    };
    // Test all combinations of parameters
    for (auto [n, e, eval_type] : fde_param) {
        DpfParameters param(n, e, eval_type);
        param.PrintParameters();
        DpfKeyGenerator gen(param);
        DpfEvaluator    eval(param);
        uint32_t        alpha = Mod(SecureRng::Rand32(), n);
        uint32_t        beta  = 1;

        // Generate keys
        Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta));
        std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        if (param.GetFdeEvalType() == EvalType::kNaive) {
            std::vector<uint32_t> outputs_0, outputs_1;
            eval.EvaluateFullDomain(keys.first, outputs_0);
            eval.EvaluateFullDomain(keys.second, outputs_1);
            std::vector<uint32_t> outputs(outputs_0.size());
            for (uint32_t i = 0; i < outputs_0.size(); ++i) {
                outputs[i] = Mod(outputs_0[i] + outputs_1[i], e);
            }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Outputs=" + ToString(outputs));
#endif
            // Check FDE
            if (!DpfFullDomainCheck(alpha, beta, outputs))
                throw oc::UnitTestFail("FDE check failed");

        } else {
            std::vector<block> outputs_0, outputs_1;
            eval.EvaluateFullDomain(keys.first, outputs_0);
            eval.EvaluateFullDomain(keys.second, outputs_1);
            std::vector<block> outputs(outputs_0.size());
            for (uint32_t i = 0; i < outputs_0.size(); ++i) {
                outputs[i] = outputs_0[i] ^ outputs_1[i];
            }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            for (uint32_t i = 0; i < outputs.size(); ++i) {
                Logger::DebugLog(LOC, "Outputs[" + std::to_string(i) + "]=" + fsswm::ToString(outputs[i], FormatType::kBin));
            }
#endif
            // Check FDE
            if (!DpfFullDomainCheckOneBit(alpha, beta, outputs))
                throw oc::UnitTestFail("FDE check failed");
        }
    }
    Logger::DebugLog(LOC, "Dpf_Fde_One_Test - Passed");
}

}    // namespace test_fsswm
