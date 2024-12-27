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

void Dpf_Params_Test() {
    std::vector<fsswm::fss::dpf::DpfParameters> params_list = {
        fsswm::fss::dpf::DpfParameters(5, 5, false),
        fsswm::fss::dpf::DpfParameters(5, 5, false),
        fsswm::fss::dpf::DpfParameters(5, 1, false),
        fsswm::fss::dpf::DpfParameters(5, 1, true),
        fsswm::fss::dpf::DpfParameters(10, 10, false),
        fsswm::fss::dpf::DpfParameters(10, 10, true),
        fsswm::fss::dpf::DpfParameters(10, 1, false),
        fsswm::fss::dpf::DpfParameters(10, 1, true),
        fsswm::fss::dpf::DpfParameters(20, 20, false),
        fsswm::fss::dpf::DpfParameters(20, 20, true),
        fsswm::fss::dpf::DpfParameters(20, 1, false),
        fsswm::fss::dpf::DpfParameters(20, 1, true),
    };

    for (const fsswm::fss::dpf::DpfParameters &params : params_list) {
        // params.PrintDpfParameters();
        uint32_t                                                    e = params.element_bitsize;
        fsswm::fss::dpf::DpfKeyGenerator                            gen(params);
        fsswm::fss::dpf::DpfEvaluator                               eval(params);
        uint32_t                                                    alpha = 5;
        uint32_t                                                    beta  = 1;
        std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys  = gen.GenerateKeys(alpha, beta);

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
             fsswm::fss::dpf::EvalType::kNaive,
             fsswm::fss::dpf::EvalType::kRec,
             fsswm::fss::dpf::EvalType::kNonRec,
             fsswm::fss::dpf::EvalType::kNonRec_HalfPRG,
             fsswm::fss::dpf::EvalType::kNonRec_ParaEnc,
         }) {

        utils::Logger::InfoLog(LOC, "Evaluation type: " + fsswm::fss::dpf::GetEvalTypeString(eval_type));

        std::vector<fsswm::fss::dpf::DpfParameters> params_list = {
            fsswm::fss::dpf::DpfParameters(5, 5, false, eval_type),
            fsswm::fss::dpf::DpfParameters(5, 1, false, eval_type),
            fsswm::fss::dpf::DpfParameters(5, 5, true, eval_type),
            fsswm::fss::dpf::DpfParameters(5, 1, true, eval_type),
            fsswm::fss::dpf::DpfParameters(10, 10, false, eval_type),
            fsswm::fss::dpf::DpfParameters(10, 1, false, eval_type),
            fsswm::fss::dpf::DpfParameters(10, 10, true, eval_type),
            fsswm::fss::dpf::DpfParameters(10, 1, true, eval_type),
            fsswm::fss::dpf::DpfParameters(11, 11, false, eval_type),
            fsswm::fss::dpf::DpfParameters(11, 1, false, eval_type),
            fsswm::fss::dpf::DpfParameters(11, 11, true, eval_type),
            fsswm::fss::dpf::DpfParameters(11, 1, true, eval_type),
            fsswm::fss::dpf::DpfParameters(20, 20, false, eval_type),
            fsswm::fss::dpf::DpfParameters(20, 1, false, eval_type),
            fsswm::fss::dpf::DpfParameters(20, 20, true, eval_type),
            fsswm::fss::dpf::DpfParameters(20, 1, true, eval_type),
        };

        for (const fsswm::fss::dpf::DpfParameters &params : params_list) {
            params.PrintDpfParameters();
            fsswm::fss::dpf::DpfKeyGenerator gen(params);
            fsswm::fss::dpf::DpfEvaluator    eval(params);
        }
    }
}

void Dpf_Fde_Test() {
    for (auto eval_type : {
             fsswm::fss::dpf::EvalType::kNaive,   // ! Not work
            //  fsswm::fss::dpf::EvalType::kRec,
            //  fsswm::fss::dpf::EvalType::kNonRec,
            //  fsswm::fss::dpf::EvalType::kNonRec_HalfPRG,
            //  fsswm::fss::dpf::EvalType::kNonRec_ParaEnc,
         }) {

        std::vector<fsswm::fss::dpf::DpfParameters> params_list = {
            fsswm::fss::dpf::DpfParameters(5, 5, false, eval_type),
            fsswm::fss::dpf::DpfParameters(10, 10, true, eval_type),
            fsswm::fss::dpf::DpfParameters(15, 15, true, eval_type),
            fsswm::fss::dpf::DpfParameters(20, 20, true, eval_type),
        };

        for (const fsswm::fss::dpf::DpfParameters &params : params_list) {
            // params.PrintDpfParameters();
            uint32_t                         n = params.input_bitsize;
            uint32_t                         e = params.element_bitsize;
            fsswm::fss::dpf::DpfKeyGenerator gen(params, true);
            fsswm::fss::dpf::DpfEvaluator    eval(params);
            uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
            uint32_t                         beta  = utils::Mod(utils::SecureRng::Rand64(), e);

            // Generate keys
            utils::Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta), true);
            std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

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
    std::vector<fsswm::fss::dpf::DpfParameters> params_list = {
        // fsswm::fss::dpf::DpfParameters(5, 1, false), // * Not supported
        fsswm::fss::dpf::DpfParameters(11, 1, true),
        fsswm::fss::dpf::DpfParameters(15, 1, true),
        fsswm::fss::dpf::DpfParameters(20, 1, true),
        fsswm::fss::dpf::DpfParameters(25, 1, true),
    };

    for (const fsswm::fss::dpf::DpfParameters &params : params_list) {
        // params.PrintDpfParameters();
        uint32_t                         n = params.input_bitsize;
        fsswm::fss::dpf::DpfKeyGenerator gen(params);
        fsswm::fss::dpf::DpfEvaluator    eval(params);
        uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
        uint32_t                         beta  = 1;

        // Generate keys
        utils::Logger::DebugLog(LOC, "alpha=" + std::to_string(alpha) + ", beta=" + std::to_string(beta), true);
        std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        for (auto eval_type : {
                 fsswm::fss::dpf::EvalType::kRec,
                 fsswm::fss::dpf::EvalType::kNonRec,
                 fsswm::fss::dpf::EvalType::kNonRec_ParaEnc,
                 fsswm::fss::dpf::EvalType::kNonRec_HalfPRG,
             }) {
            std::vector<block> outputs_0, outputs_1;
            eval.EvaluateFullDomainOneBit(keys.first, outputs_0);
            eval.EvaluateFullDomainOneBit(keys.second, outputs_1);
            std::vector<block> outputs(outputs_0.size());
            for (uint32_t i = 0; i < outputs_0.size(); i++) {
                outputs[i] = outputs_0[i] ^ outputs_1[i];
            }

#ifdef LOG_LEVEL_DEBUG
            for (uint32_t i = 0; i < outputs.size(); i++) {
                utils::Logger::InfoLog(LOC, "Outputs[" + std::to_string(i) + "]=" + fsswm::fss::ToString(outputs[i], fsswm::fss::FormatType::kBin));
            }
#endif

            // Check FDE
            if (!DpfFullDomainCheckOneBit(alpha, beta, outputs))
                throw oc::UnitTestFail("FDE check failed");
        }
    }
}

void Dpf_Fde_Bench_Test() {
    for (auto eval_type : {
             //  fsswm::fss::dpf::EvalType::kRec,
             //  fsswm::fss::dpf::EvalType::kNonRec,
             //  fsswm::fss::dpf::EvalType::kNonRec_HalfPRG,
             fsswm::fss::dpf::EvalType::kNonRec_ParaEnc,
             //  fsswm::fss::dpf::EvalType::kNaive,
         }) {
        for (uint32_t size = 8; size <= 10; size += 1) {
            fsswm::fss::dpf::DpfParameters params(size, size, true);
            if (eval_type == fsswm::fss::dpf::EvalType::kNaive) {
                params.enable_early_termination = false;
            }
            uint32_t                         n = params.input_bitsize;
            uint32_t                         e = params.element_bitsize;
            fsswm::fss::dpf::DpfKeyGenerator gen(params, true);
            fsswm::fss::dpf::DpfEvaluator    eval(params);
            uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
            uint32_t                         beta  = utils::Mod(utils::SecureRng::Rand64(), e);

            // Generate keys
            std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

            // Evaluate keys

            utils::TimerManager timer_mgr;
            int32_t             timer_id = timer_mgr.CreateNewTimer("FDE Benchmark");
            timer_mgr.SelectTimer(timer_id);

            uint32_t repeat = 10;

            for (uint32_t i = 0; i < repeat; i++) {
                timer_mgr.Start();
                std::vector<uint32_t> outputs_0;
                eval.EvaluateFullDomain(keys.first, outputs_0);
                timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
            }

            timer_mgr.PrintCurrentResults("n=" + std::to_string(size) + "[" + fsswm::fss::dpf::GetEvalTypeString(eval_type) + "]", utils::TimeUnit::MICROSECONDS);
        }
    }
}

void Dpf_Fde_One_Bench_Test() {
    for (uint32_t size = 11; size <= 28; size += 1) {
        fsswm::fss::dpf::DpfParameters   params(size, 1, true);
        uint32_t                         n = params.input_bitsize;
        fsswm::fss::dpf::DpfKeyGenerator gen(params);
        fsswm::fss::dpf::DpfEvaluator    eval(params);
        uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
        uint32_t                         beta  = 1;

        // Generate keys
        std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        for (auto eval_type : {
                 fsswm::fss::dpf::EvalType::kRec,
                 fsswm::fss::dpf::EvalType::kNonRec,
                 fsswm::fss::dpf::EvalType::kNonRec_HalfPRG,
                 //  fsswm::fss::dpf::EvalType::kNonRec_ParaEnc,
             }) {

            utils::TimerManager timer_mgr;
            int32_t             timer_id = timer_mgr.CreateNewTimer("FDE Benchmark");
            timer_mgr.SelectTimer(timer_id);

            uint32_t repeat = 10;

            for (uint32_t i = 0; i < repeat; i++) {
                timer_mgr.Start();
                std::vector<block> outputs_0;
                eval.EvaluateFullDomainOneBit(keys.first, outputs_0);
                timer_mgr.Stop("n=" + std::to_string(size) + " (" + std::to_string(i) + ")");
            }

            timer_mgr.PrintCurrentResults("n=" + std::to_string(size) + "[" + fsswm::fss::dpf::GetEvalTypeString(eval_type) + "]", utils::TimeUnit::MICROSECONDS);
        }
    }
}

}    // namespace test_fsswm
