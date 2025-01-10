#include "prg_test.h"

#include "FssWM/fss/fss.h"
#include "FssWM/fss/prg.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/timer.h"
#include "FssWM/utils/utils.h"

namespace test_fsswm {

using fsswm::fss::block;
using fsswm::fss::Equal;
using fsswm::fss::ToString;
using fsswm::utils::Logger;

void Prg_Test() {
    Logger::DebugLog(LOC, "Prg_Test...");

    fsswm::fss::prg::PseudoRandomGenerator prg = fsswm::fss::prg::PseudoRandomGeneratorSingleton::GetInstance();
    Logger::DebugLog(LOC, "PseudoRandomGenerator created successfully");

    block                seed_in = fsswm::fss::makeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    std::array<block, 2> seed_out;
    prg.DoubleExpand(seed_in, seed_out);

    Logger::DebugLog(LOC, "seed_in: " + ToString(seed_in));
    Logger::DebugLog(LOC, "seed_out[0]: " + ToString(seed_out[0]));
    Logger::DebugLog(LOC, "seed_out[1]: " + ToString(seed_out[1]));

    block expanded_seed;
    bool  bit = false;
    prg.Expand(seed_in, expanded_seed, bit);

    Logger::DebugLog(LOC, "expanded_seed: " + ToString(expanded_seed));
    Logger::DebugLog(LOC, "Equal(seed_out[0], expanded_seed): " + std::to_string(Equal(seed_out[0], expanded_seed)));

    bit = true;
    prg.Expand(seed_in, expanded_seed, bit);

    Logger::DebugLog(LOC, "expanded_seed: " + ToString(expanded_seed));
    Logger::DebugLog(LOC, "Equal(seed_out[1], expanded_seed): " + std::to_string(Equal(seed_out[1], expanded_seed)));

    Logger::DebugLog(LOC, "PRG test finished");

    std::array<block, 8> seed_in_array;
    for (size_t i = 0; i < 8; ++i) {
        seed_in_array[i] = fsswm::fss::makeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    }
    std::array<std::array<block, 8>, 2> seed_out_array;
    prg.DoubleExpand(seed_in_array, seed_out_array);

    for (size_t i = 0; i < 8; ++i) {
        Logger::DebugLog(LOC, "seed_in_array[" + std::to_string(i) + "]: " + ToString(seed_in_array[i]));
        Logger::DebugLog(LOC, "seed_out_array[0][" + std::to_string(i) + "]: " + ToString(seed_out_array[0][i]));
        Logger::DebugLog(LOC, "seed_out_array[1][" + std::to_string(i) + "]: " + ToString(seed_out_array[1][i]));
    }

    Logger::DebugLog(LOC, "Prg_Test - Passed");
}

}    // namespace test_fsswm
