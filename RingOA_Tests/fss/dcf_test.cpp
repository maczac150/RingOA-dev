#include "dcf_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/fss/dcf_eval.h"
#include "FssWM/fss/dcf_gen.h"
#include "FssWM/fss/dcf_key.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace {

bool DcfFullDomainCheck(const uint64_t alpha, const uint64_t beta, const std::vector<uint64_t> &res) {
    bool check = true;
    for (uint64_t i = 0; i < res.size(); ++i) {
        if ((i < alpha && res[i] == beta) || (i >= alpha && res[i] == 0)) {
            check &= true;
        } else {
            check &= false;
            fsswm::Logger::DebugLog(LOC, "FDE check failed at x=" + fsswm::ToString(i) + " -> Result: " + fsswm::ToString(res[i]));
        }
    }
    return check;
}

}    // namespace

namespace test_fsswm {

using fsswm::block;
using fsswm::FormatType;
using fsswm::GlobalRng;
using fsswm::Logger;
using fsswm::Mod;
using fsswm::TimerManager;
using fsswm::ToString, fsswm::Format;
using fsswm::fss::EvalType, fsswm::fss::OutputType;
using fsswm::fss::dcf::DcfEvaluator;
using fsswm::fss::dcf::DcfKey;
using fsswm::fss::dcf::DcfKeyGenerator;
using fsswm::fss::dcf::DcfParameters;

void Dcf_EvalAt_Test() {
    Logger::DebugLog(LOC, "Dcf_EvalAt_Test...");
    // Test parameters
    const std::vector<std::pair<uint64_t, uint64_t>> size_pair = {
        {3, 3},
        // {10, 10},
        // {10, 1},
    };

    // Test all combinations of parameters
    for (auto [n, e] : size_pair) {
        DcfParameters param(n, e);
        param.PrintParameters();
        DcfKeyGenerator           gen(param);
        DcfEvaluator              eval(param);
        uint64_t                  alpha = 5;
        uint64_t                  beta  = 1;
        std::pair<DcfKey, DcfKey> keys  = gen.GenerateKeys(alpha, beta);

        uint64_t x   = 3;
        uint64_t y_0 = eval.EvaluateAt(keys.first, x);
        uint64_t y_1 = eval.EvaluateAt(keys.second, x);
        uint64_t y   = Mod(y_0 + y_1, e);

        if (y != beta)
            throw osuCrypto::UnitTestFail("y is not equal to beta");

        x   = 7;
        y_0 = eval.EvaluateAt(keys.first, x);
        y_1 = eval.EvaluateAt(keys.second, x);
        y   = Mod(y_0 + y_1, e);

        if (y != 0)
            throw osuCrypto::UnitTestFail("y is not equal to 0");
    }

    Logger::DebugLog(LOC, "Dcf_EvalAt_Test - Passed");
}

void Dcf_Fde_Test() {
    Logger::DebugLog(LOC, "Dcf_Fde_Test...");
    const std::vector<std::tuple<uint64_t, uint64_t>> size_pair = {
        {3, 3},
    };

    // Test all combinations of parameters
    for (auto [n, e] : size_pair) {
        DcfParameters param(n, e);
        param.PrintParameters();
        DcfKeyGenerator gen(param);
        DcfEvaluator    eval(param);
        uint64_t        alpha = Mod(GlobalRng::Rand<uint64_t>(), n);
        uint64_t        beta  = Mod(GlobalRng::Rand<uint64_t>(), e);

        // Generate keys
        Logger::DebugLog(LOC, "alpha=" + ToString(alpha) + ", beta=" + ToString(beta));
        std::pair<DcfKey, DcfKey> keys = gen.GenerateKeys(alpha, beta);

        // Evaluate keys
        std::vector<uint64_t> outputs_0(1 << n), outputs_1(1 << n);

        for (uint64_t i = 0; i < outputs_0.size(); ++i) {
            outputs_0[i] = eval.EvaluateAt(keys.first, i);
            outputs_1[i] = eval.EvaluateAt(keys.second, i);
        }

        std::vector<uint64_t> outputs(outputs_0.size());
        for (uint64_t i = 0; i < outputs_0.size(); ++i) {
            outputs[i] = Mod(outputs_0[i] + outputs_1[i], e);
        }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Outputs=" + ToString(outputs));
#endif

        // Check FDE
        if (!DcfFullDomainCheck(alpha, beta, outputs))
            throw osuCrypto::UnitTestFail("FDE check failed");
    }
    Logger::DebugLog(LOC, "Dcf_Fde_Test - Passed");
}

}    // namespace test_fsswm
