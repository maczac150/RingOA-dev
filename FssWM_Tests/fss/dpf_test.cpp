#include "dpf_test.h"

#include "cryptoTools/Common/Defines.h"
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
using fsswm::fss::dpf::DpfEvaluator;
using fsswm::fss::dpf::DpfKey;
using fsswm::fss::dpf::DpfKeyGenerator;
using fsswm::fss::dpf::DpfParameters;
using fsswm::fss::dpf::EvalType;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::SecureRng;
using fsswm::TimerManager;
using fsswm::ToString;
using fsswm::FormatType;

void Dpf_Params_Test() {
    // Test parameters
    std::vector<DpfParameters> params_list = {
        DpfParameters(5, 5, false),
        DpfParameters(5, 5, true),
        DpfParameters(5, 1, false),
        DpfParameters(5, 1, true),
        DpfParameters(10, 10, false),
        DpfParameters(10, 10, true),
        DpfParameters(10, 1, false),
        DpfParameters(10, 1, true),
        DpfParameters(20, 20, false),
        DpfParameters(20, 20, true),
        DpfParameters(20, 1, false),
        DpfParameters(20, 1, true),
    };

    for (const DpfParameters &params : params_list) {
        params.PrintDpfParameters();
        DpfKeyGenerator gen(params);
        DpfEvaluator    eval(params);
    }
}

void Dpf_EvalAt_Test() {
    // Test parameters
    std::vector<DpfParameters> params_list = {
        DpfParameters(5, 5, false),
        DpfParameters(5, 5, true),
        DpfParameters(5, 1, false),
        DpfParameters(5, 1, true),
        DpfParameters(10, 10, false),
        DpfParameters(10, 10, true),
        DpfParameters(10, 1, false),
        DpfParameters(10, 1, true),
        DpfParameters(20, 20, false),
        DpfParameters(20, 20, true),
        DpfParameters(20, 1, false),
        DpfParameters(20, 1, true),
    };

    for (const DpfParameters &params : params_list) {
        params.PrintDpfParameters();
        uint32_t                  e = params.GetOutputBitsize();
        DpfKeyGenerator           gen(params);
        DpfEvaluator              eval(params);
        uint32_t                  alpha = 5;
        uint32_t                  beta  = 1;
        std::pair<DpfKey, DpfKey> keys  = gen.GenerateKeys(alpha, beta);

        uint32_t x   = 5;
        uint32_t y_0 = eval.EvaluateAt(keys.first, x);
        uint32_t y_1 = eval.EvaluateAt(keys.second, x);
        uint32_t y   = Mod(y_0 + y_1, e);

        if (y != beta)
            throw oc::UnitTestFail("y is not equal to beta");

        x   = 10;
        y_0 = eval.EvaluateAt(keys.first, x);
        y_1 = eval.EvaluateAt(keys.second, x);
        y   = Mod(y_0 + y_1, e);

        if (y != 0)
            throw oc::UnitTestFail("y is not equal to 0");
    }
}

void Dpf_Fde_Type_Test() {
    for (auto eval_type : {
             EvalType::kNaive,
             EvalType::kRecursion,
             EvalType::kIterSingle,
             EvalType::kIterDouble,
             EvalType::kIterSingleBatch,
             EvalType::kIterDoubleBatch,
         }) {

        Logger::DebugLog(LOC, "Evaluation type: " + GetEvalTypeString(eval_type));

        std::vector<DpfParameters> params_list = {
            DpfParameters(5, 5, false, eval_type),
            DpfParameters(5, 1, false, eval_type),
            DpfParameters(5, 5, true, eval_type),
            DpfParameters(5, 1, true, eval_type),
            DpfParameters(10, 10, false, eval_type),
            DpfParameters(10, 1, false, eval_type),
            DpfParameters(10, 10, true, eval_type),
            DpfParameters(10, 1, true, eval_type),
            DpfParameters(11, 11, false, eval_type),
            DpfParameters(11, 1, false, eval_type),
            DpfParameters(11, 11, true, eval_type),
            DpfParameters(11, 1, true, eval_type),
            DpfParameters(20, 20, false, eval_type),
            DpfParameters(20, 1, false, eval_type),
            DpfParameters(20, 20, true, eval_type),
            DpfParameters(20, 1, true, eval_type),
        };

        for (const DpfParameters &params : params_list) {
            params.PrintDpfParameters();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
        }
    }
}

void Dpf_Fde_Test() {
    for (auto eval_type : {
             EvalType::kNaive,
             EvalType::kRecursion,
             EvalType::kIterSingle,
             EvalType::kIterDouble,
             EvalType::kIterSingleBatch,
             EvalType::kIterDoubleBatch,
         }) {

        std::vector<DpfParameters> params_list = {
            DpfParameters(5, 5, true, eval_type),
            DpfParameters(10, 10, true, eval_type),
            DpfParameters(15, 15, true, eval_type),
            DpfParameters(20, 20, true, eval_type),
        };

        for (const DpfParameters &params : params_list) {
            params.PrintDpfParameters();
            uint32_t        n = params.GetInputBitsize();
            uint32_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
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
    }
}

void Dpf_Fde_One_Test() {
    for (auto eval_type : {
             //  EvalType::kNaive, // * Not supported
             EvalType::kRecursion,
             EvalType::kIterSingle,
             EvalType::kIterDouble,
             EvalType::kIterSingleBatch,
             EvalType::kIterDoubleBatch,
         }) {

        std::vector<DpfParameters> params_list = {
            // DpfParameters(5, 1, false), // * Not supported
            DpfParameters(11, 1, true, eval_type),
            DpfParameters(15, 1, true, eval_type),
            DpfParameters(20, 1, true, eval_type),
            DpfParameters(25, 1, true, eval_type),
        };

        for (const DpfParameters &params : params_list) {
            params.PrintDpfParameters();
            uint32_t        n = params.GetInputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint32_t        alpha = Mod(SecureRng::Rand32(), n);
            uint32_t        beta  = 1;

            // Generate keys
            Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta));
            std::pair<DpfKey, DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys
            std::vector<block> outputs_0, outputs_1;
            eval.EvaluateFullDomainOneBit(keys.first, outputs_0);
            eval.EvaluateFullDomainOneBit(keys.second, outputs_1);
            std::vector<block> outputs(outputs_0.size());
            for (uint32_t i = 0; i < outputs_0.size(); ++i) {
                outputs[i] = outputs_0[i] ^ outputs_1[i];
            }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            for (uint32_t i = 0; i < outputs.size(); ++i) {
                Logger::InfoLog(LOC, "Outputs[" + std::to_string(i) + "]=" + fsswm::ToString(outputs[i], FormatType::kBin));
            }
#endif

            // Check FDE
            if (!DpfFullDomainCheckOneBit(alpha, beta, outputs))
                throw oc::UnitTestFail("FDE check failed");
        }
    }
}

void Dpf_Fde_TwoKey_Test() {
    for (auto eval_type : {
             EvalType::kIterSingleBatch_2Keys,
         }) {

        std::vector<DpfParameters> params_list = {
            DpfParameters(10, 10, true, eval_type),
            DpfParameters(15, 15, true, eval_type),
            DpfParameters(20, 20, true, eval_type),
        };

        for (const DpfParameters &params : params_list) {
            params.PrintDpfParameters();
            uint32_t        n = params.GetInputBitsize();
            uint32_t        e = params.GetOutputBitsize();
            DpfKeyGenerator gen(params);
            DpfEvaluator    eval(params);
            uint32_t        alpha1 = Mod(SecureRng::Rand32(), n);
            uint32_t        beta1  = Mod(SecureRng::Rand32(), e);
            uint32_t        alpha2 = Mod(SecureRng::Rand32(), n);
            uint32_t        beta2  = Mod(SecureRng::Rand32(), e);

            // Generate keys
            Logger::DebugLog(LOC, "alpha1=" + std::to_string(alpha1) + ", beta1=" + std::to_string(beta1));
            Logger::DebugLog(LOC, "alpha2=" + std::to_string(alpha2) + ", beta2=" + std::to_string(beta2));
            std::pair<DpfKey, DpfKey> keys1 = gen.GenerateKeys(alpha1, beta1);
            std::pair<DpfKey, DpfKey> keys2 = gen.GenerateKeys(alpha2, beta2);

            // Evaluate keys
            std::vector<uint32_t> out1_0, out1_1, out2_0, out2_1;
            eval.EvaluateFullDomainTwoKeys(keys1.first, keys2.first, out1_0, out2_0);
            eval.EvaluateFullDomainTwoKeys(keys1.second, keys2.second, out1_1, out2_1);

            std::vector<uint32_t> out1(out1_0.size()), out2(out2_0.size());
            for (uint32_t i = 0; i < out1_0.size(); ++i) {
                out1[i] = Mod(out1_0[i] + out1_1[i], e);
                out2[i] = Mod(out2_0[i] + out2_1[i], e);
            }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "Outputs1=" + ToString(out1));
            Logger::DebugLog(LOC, "Outputs2=" + ToString(out2));
#endif

            // Check FDE
            if (!DpfFullDomainCheck(alpha1, beta1, out1) || !DpfFullDomainCheck(alpha2, beta2, out2))
                throw oc::UnitTestFail("FDE check failed");
        }
    }
}

}    // namespace test_fsswm
