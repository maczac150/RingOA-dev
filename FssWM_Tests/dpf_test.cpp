#include "FssWM/fss/dpf_eval.hpp"
#include "FssWM/fss/dpf_gen.hpp"
#include "FssWM/fss/dpf_key.hpp"
#include "FssWM/utils/logger.hpp"
#include "FssWM/utils/utils.hpp"

namespace fsswm {
namespace test {

void dpf_test() {
    utils::Logger::InfoLog(LOCATION, "DPF test started");
    uint32_t                  n = 5;
    uint32_t                  e = 5;
    fss::dpf::DpfParameters   params(n, e, false);
    fss::dpf::DpfKeyGenerator gen(params, true);
    fss::dpf::DpfEvaluator    eval(params, true);

    uint32_t                                      alpha = 5;
    uint32_t                                      beta  = 5;
    std::pair<fss::dpf::DpfKey, fss::dpf::DpfKey> keys  = gen.GenerateKeys(alpha, beta);

    for (uint32_t i = 0; i < (1 << n); i++) {
        uint32_t x   = i;
        uint32_t y_0 = eval.EvaluateAt(keys.first, x);
        uint32_t y_1 = eval.EvaluateAt(keys.second, x);

        utils::Logger::InfoLog(LOCATION, "x=" + std::to_string(x) + "-> y=" + std::to_string(utils::Mod(y_0 + y_1, e)) + " (y_0=" + std::to_string(y_0) + ", y_1=" + std::to_string(y_1) + ")");
    }
}

}    // namespace test
}    // namespace fsswm
