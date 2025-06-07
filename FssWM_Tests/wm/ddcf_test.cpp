#include "ddcf_test.h"

#include <cryptoTools/Common/TestCollection.h>

#include "FssWM/sharing/binary_2p.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"
#include "FssWM/wm/ddcf.h"

namespace {

bool DdcfFullDomainCheck(const uint64_t alpha, const uint64_t beta_1, const uint64_t beta_2, const std::vector<uint64_t> &res) {
    bool check = true;
    for (uint64_t i = 0; i < res.size(); ++i) {
        if ((i < alpha && res[i] == beta_1) || (i >= alpha && res[i] == beta_2)) {
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
using fsswm::sharing::BinarySharing2P;
using fsswm::wm::DdcfEvaluator;
using fsswm::wm::DdcfKey;
using fsswm::wm::DdcfKeyGenerator;
using fsswm::wm::DdcfParameters;

void Ddcf_EvalAt_Test() {
    Logger::DebugLog(LOC, "Ddcf_EvalAt_Test...");
    // Test parameters
    const std::vector<std::pair<uint64_t, uint64_t>> size_pair = {
        {3, 3},
        // {10, 10},
        // {10, 1},
    };

    // Test all combinations of parameters
    for (auto [n, e] : size_pair) {
        DdcfParameters param(n, e);
        param.PrintParameters();
        BinarySharing2P             bss(e);
        DdcfKeyGenerator            gen(param, bss);
        DdcfEvaluator               eval(param, bss);
        uint64_t                    alpha  = 5;
        uint64_t                    beta_1 = 1;
        uint64_t                    beta_2 = 2;
        std::pair<DdcfKey, DdcfKey> keys   = gen.GenerateKeys(alpha, beta_1, beta_2);

        uint64_t x   = 3;
        uint64_t y_0 = eval.EvaluateAt(keys.first, x);
        uint64_t y_1 = eval.EvaluateAt(keys.second, x);
        uint64_t y   = Mod(y_0 + y_1, e);

        if (y != beta_1)
            throw osuCrypto::UnitTestFail("y is not equal to beta_1");

        x   = 7;
        y_0 = eval.EvaluateAt(keys.first, x);
        y_1 = eval.EvaluateAt(keys.second, x);
        y   = Mod(y_0 + y_1, e);

        if (y != beta_2)
            throw osuCrypto::UnitTestFail("y is not equal to beta_2");
    }

    Logger::DebugLog(LOC, "Ddcf_EvalAt_Test - Passed");
}

void Ddcf_Fde_Test() {
    Logger::DebugLog(LOC, "Ddcf_Fde_Test...");
    const std::vector<std::tuple<uint64_t, uint64_t>> size_pair = {
        {3, 3},
    };

    // Test all combinations of parameters
    for (auto [n, e] : size_pair) {
        DdcfParameters param(n, e);
        param.PrintParameters();
        BinarySharing2P  bss(e);
        DdcfKeyGenerator gen(param, bss);
        DdcfEvaluator    eval(param, bss);
        uint64_t         alpha  = 5;
        uint64_t         beta_1 = 1;
        uint64_t         beta_2 = 2;

        // Generate keys
        Logger::DebugLog(LOC, "alpha=" + ToString(alpha) + ", beta_1=" + ToString(beta_1) + ", beta_2=" + ToString(beta_2));
        std::pair<DdcfKey, DdcfKey> keys = gen.GenerateKeys(alpha, beta_1, beta_2);

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
        if (!DdcfFullDomainCheck(alpha, beta_1, beta_2, outputs))
            throw osuCrypto::UnitTestFail("FDE check failed");
    }
    Logger::DebugLog(LOC, "Ddcf_Fde_Test - Passed");
}

}    // namespace test_fsswm
