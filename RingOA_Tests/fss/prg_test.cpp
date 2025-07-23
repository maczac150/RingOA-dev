#include "prg_test.h"

#include "FssWM/fss/fss.h"
#include "FssWM/fss/prg.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace test_fsswm {

using fsswm::block;
using fsswm::Logger;
using fsswm::ToString, fsswm::Format;

void Prg_Test() {
    Logger::DebugLog(LOC, "Prg_Test...");

    fsswm::fss::prg::PseudoRandomGenerator prg = fsswm::fss::prg::PseudoRandomGenerator::GetInstance();
    Logger::DebugLog(LOC, "PseudoRandomGenerator created successfully");

    block                seed_in = fsswm::MakeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    std::array<block, 2> seed_out;
    prg.DoubleExpand(seed_in, seed_out);

    Logger::DebugLog(LOC, "seed_in: " + Format(seed_in));
    Logger::DebugLog(LOC, "seed_out[0]: " + Format(seed_out[0]));
    Logger::DebugLog(LOC, "seed_out[1]: " + Format(seed_out[1]));

    block expanded_seed;
    bool  bit = false;
    prg.Expand(seed_in, expanded_seed, bit);

    Logger::DebugLog(LOC, "expanded_seed: " + Format(expanded_seed));
    Logger::DebugLog(LOC, "Equal(seed_out[0], expanded_seed): " + ToString(seed_out[0] == expanded_seed));

    bit = true;
    prg.Expand(seed_in, expanded_seed, bit);

    Logger::DebugLog(LOC, "expanded_seed: " + Format(expanded_seed));
    Logger::DebugLog(LOC, "Equal(seed_out[1], expanded_seed): " + ToString(seed_out[1] == expanded_seed));

    Logger::DebugLog(LOC, "Prg_Test - Passed");
}

}    // namespace test_fsswm
