#include "FssWM/fss/fss.hpp"
#include "FssWM/fss/prg.hpp"
#include "FssWM/utils/logger.hpp"
#include "FssWM/utils/timer.hpp"

namespace test_fsswm {

void Prg_Test() {
    utils::Logger::InfoLog(LOC, "PRG test started");
    utils::Logger::InfoLog(LOC, "kPrgKeySeedLeft: " + fsswm::fss::ToString(fsswm::fss::prg::kPrgKeySeedLeft));
    utils::Logger::InfoLog(LOC, "kPrgKeySeedRight: " + fsswm::fss::ToString(fsswm::fss::prg::kPrgKeySeedRight));

    fsswm::fss::prg::PseudoRandomGenerator prg = fsswm::fss::prg::PseudoRandomGeneratorSingleton::GetInstance();
    utils::Logger::InfoLog(LOC, "PseudoRandomGenerator created");

    block                seed_in = makeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    std::array<block, 2> seed_out;
    prg.DoubleExpand(seed_in, seed_out);
    utils::Logger::InfoLog(LOC, "DoubleExpand(seed_in, seed_out) called");

    utils::Logger::InfoLog(LOC, "seed_in: " + fsswm::fss::ToString(seed_in));
    utils::Logger::InfoLog(LOC, "seed_out[0]: " + fsswm::fss::ToString(seed_out[0]));
    utils::Logger::InfoLog(LOC, "seed_out[1]: " + fsswm::fss::ToString(seed_out[1]));

    block expanded_seed;
    bool  bit = false;
    prg.Expand(seed_in, expanded_seed, bit);

    utils::Logger::InfoLog(LOC, "expanded_seed: " + fsswm::fss::ToString(expanded_seed));
    utils::Logger::InfoLog(LOC, "Equal(seed_out[0], expanded_seed): " + std::to_string(fsswm::fss::Equal(seed_out[0], expanded_seed)));

    bit = true;
    prg.Expand(seed_in, expanded_seed, bit);

    utils::Logger::InfoLog(LOC, "expanded_seed: " + fsswm::fss::ToString(expanded_seed));
    utils::Logger::InfoLog(LOC, "Equal(seed_out[1], expanded_seed): " + std::to_string(fsswm::fss::Equal(seed_out[1], expanded_seed)));

    utils::Logger::InfoLog(LOC, "PRG test finished");

    // utils::Logger::InfoLog(LOC, "PRG microbenchmark started");

    // uint32_t              repeat = 1 << 20;
    // utils::ExecutionTimer timer;
    // timer.SetTimeUnit(utils::TimeUnit::MICROSECONDS);

    // timer.Start();
    // for (uint32_t i = 0; i < repeat; i++) {
    //     prg.DoubleExpand(seed_in, seed_out);
    // }
    // timer.Print(LOC, "DoubleExpand(seed_in, seed_out) elapsed time: ");

    // timer.Start();
    // for (uint32_t i = 0; i < repeat; i++) {
    //     prg.Expand(seed_in, expanded_seed, false);
    //     prg.Expand(seed_in, expanded_seed, true);
    // }
    // timer.Print(LOC, "Expand(seed_in, expanded_seed, bit) elapsed time: ");

    // utils::Logger::InfoLog(LOC, "PRG microbenchmark finished");
}

}    // namespace test_fsswm
