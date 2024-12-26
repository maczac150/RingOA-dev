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

bool DpfFullDomainCheck(const uint32_t alpha, const uint32_t beta, const std::vector<uint32_t> &res, const bool debug) {
    bool check = true;
    for (uint32_t i = 0; i < res.size(); i++) {
        if ((i == alpha && res[i] == beta) || (i != alpha && res[i] == 0)) {
            check &= true;
        } else {
            check &= false;
            utils::Logger::DebugLog(LOC, "FDE check failed at x=" + std::to_string(i) + " -> Result: " + std::to_string(res[i]), debug);
        }
    }
    return check;
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

void Dpf_Fde_Test() {
    std::vector<fsswm::fss::dpf::DpfParameters> params_list = {
        fsswm::fss::dpf::DpfParameters(5, 5, false),
        fsswm::fss::dpf::DpfParameters(10, 10, true),
        fsswm::fss::dpf::DpfParameters(15, 15, true),
        fsswm::fss::dpf::DpfParameters(20, 20, true),
        fsswm::fss::dpf::DpfParameters(25, 25, true),
    };

    for (const fsswm::fss::dpf::DpfParameters &params : params_list) {
        // params.PrintDpfParameters();
        uint32_t                         n = params.input_bitsize;
        uint32_t                         e = params.element_bitsize;
        fsswm::fss::dpf::DpfKeyGenerator gen(params);
        fsswm::fss::dpf::DpfEvaluator    eval(params, true);
        uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
        uint32_t                         beta  = utils::Mod(utils::SecureRng::Rand64(), e);

        // Generate keys
        std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        std::vector<uint32_t> outputs_0 = eval.EvaluateFullDomain(keys.first);
        std::vector<uint32_t> outputs_1 = eval.EvaluateFullDomain(keys.second);
        std::vector<uint32_t> outputs(outputs_0.size());
        for (uint32_t i = 0; i < outputs_0.size(); i++) {
            outputs[i] = utils::Mod(outputs_0[i] + outputs_1[i], e);
        }

        // Check FDE
        if (!DpfFullDomainCheck(alpha, beta, outputs, true))
            throw oc::UnitTestFail("FDE check failed");
    }
}

void Dpf_Fde_One_Test() {
    std::vector<fsswm::fss::dpf::DpfParameters> params_list = {
        fsswm::fss::dpf::DpfParameters(5, 1, false),
        fsswm::fss::dpf::DpfParameters(10, 1, true),
        fsswm::fss::dpf::DpfParameters(15, 1, true),
        fsswm::fss::dpf::DpfParameters(20, 1, true),
        fsswm::fss::dpf::DpfParameters(25, 1, true),
    };

    for (const fsswm::fss::dpf::DpfParameters &params : params_list) {
        // params.PrintDpfParameters();
        uint32_t                         n = params.input_bitsize;
        uint32_t                         e = params.element_bitsize;
        fsswm::fss::dpf::DpfKeyGenerator gen(params);
        fsswm::fss::dpf::DpfEvaluator    eval(params, true);
        uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
        uint32_t                         beta  = utils::Mod(utils::SecureRng::Rand64(), e);

        // Generate keys
        std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        std::vector<uint32_t> outputs_0 = eval.EvaluateFullDomainOneBit(keys.first);
        std::vector<uint32_t> outputs_1 = eval.EvaluateFullDomainOneBit(keys.second);
        std::vector<uint32_t> outputs(outputs_0.size());
        for (uint32_t i = 0; i < outputs_0.size(); i++) {
            outputs[i] = utils::Mod(outputs_0[i] + outputs_1[i], e);
        }

        // Check FDE
        if (!DpfFullDomainCheck(alpha, beta, outputs, true))
            throw oc::UnitTestFail("FDE check failed");
    }
}

void Dpf_Fde_Bench_Test() {
    // utils::ExecutionTimer timer;
    // timer.SetTimeUnit(utils::TimeUnit::MICROSECONDS);

    uint32_t            repeat = 30;
    std::vector<double> eval_times(repeat);

    for (uint32_t size = 12; size <= 28; size += 2) {
        fsswm::fss::dpf::DpfParameters   params(size, size, true);
        uint32_t                         n = params.input_bitsize;
        uint32_t                         e = params.element_bitsize;
        fsswm::fss::dpf::DpfKeyGenerator gen(params);
        fsswm::fss::dpf::DpfEvaluator    eval(params, true);
        uint32_t                         alpha = utils::Mod(utils::SecureRng::Rand64(), n);
        uint32_t                         beta  = utils::Mod(utils::SecureRng::Rand64(), e);

        // Generate keys
        std::pair<fsswm::fss::dpf::DpfKey, fsswm::fss::dpf::DpfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        for (uint32_t i = 0; i < repeat; i++) {
            // timer.Start();
            std::vector<uint32_t> outputs_0 = eval.EvaluateFullDomain(keys.first);
            // eval_times[i]                   = timer.Print(LOC, "FDE evaluation time for size " + std::to_string(size));
        }
    }
}

}    // namespace test_fsswm
