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

    block zero_block = makeBlock(0, 0);
    block one_block  = makeBlock(1, 0);
    utils::Logger::InfoLog(LOC, "Equal test: zero_block == one_block: " + std::to_string(fsswm::fss::Equal(zero_block, one_block)));
    utils::Logger::InfoLog(LOC, "Equal test: zero_block == zero_block: " + std::to_string(fsswm::fss::Equal(zero_block, zero_block)));

    uint32_t              repeat = 1 << 20;
    utils::ExecutionTimer timer;
    timer.SetTimeUnit(utils::TimeUnit::MICROSECONDS);
    timer.Start();
    for (uint32_t i = 0; i < repeat; i++) {
        prg.DoubleExpand(seed_in, seed_out);
    }
    timer.Print(LOC, "DoubleExpand(seed_in, seed_out) elapsed time: ");
}

}    // namespace test_fsswm
