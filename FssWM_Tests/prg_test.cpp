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
    utils::Logger::InfoLog(LOC, "PseudoRandomGenerator created successfully");

    block                seed_in = makeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    std::array<block, 2> seed_out;
    prg.DoubleExpand(seed_in, seed_out);

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

    std::array<block, 8> seed_in_array;
    for (size_t i = 0; i < 8; ++i) {
        seed_in_array[i] = makeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    }
    std::array<std::array<block, 8>, 2> seed_out_array;
    prg.DoubleExpand(seed_in_array, seed_out_array);

    for (size_t i = 0; i < 8; ++i) {
        utils::Logger::InfoLog(LOC, "seed_in_array[" + std::to_string(i) + "]: " + fsswm::fss::ToString(seed_in_array[i]));
        utils::Logger::InfoLog(LOC, "seed_out_array[0][" + std::to_string(i) + "]: " + fsswm::fss::ToString(seed_out_array[0][i]));
        utils::Logger::InfoLog(LOC, "seed_out_array[1][" + std::to_string(i) + "]: " + fsswm::fss::ToString(seed_out_array[1][i]));
    }

    // ############################################################
    // ############################################################
    // ############################################################

    utils::Logger::InfoLog(LOC, "PRG microbenchmark started");

    utils::TimerManager timer_mgr;
    int32_t             timer_id = timer_mgr.CreateNewTimer("PRG microbenchmark");
    timer_mgr.SelectTimer(timer_id);

    uint32_t repeat = 1 << 24;
    utils::Logger::InfoLog(LOC, "repeat: " + std::to_string(repeat));

    timer_mgr.Start();
    for (uint32_t i = 0; i < repeat; i++) {
        prg.DoubleExpand(seed_in, seed_out);
    }
    timer_mgr.Stop("DoubleExpand");

    timer_mgr.Start();
    for (uint32_t i = 0; i < repeat; i++) {
        prg.Expand(seed_in, expanded_seed, false);
        prg.Expand(seed_in, expanded_seed, true);
    }
    timer_mgr.Stop("2 calls of Expand");

    timer_mgr.PrintCurrentResults("", utils::TimeUnit::MICROSECONDS);

    utils::Logger::InfoLog(LOC, "PRG microbenchmark finished");
}

}    // namespace test_fsswm
