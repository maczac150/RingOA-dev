#include "FssWM/fss/fss.hpp"
#include "FssWM/fss/prg.hpp"
#include "FssWM/utils/logger.hpp"
#include "FssWM/utils/timer.hpp"

namespace fsswm {
namespace test {

void prg_test() {
    utils::Logger::InfoLog(LOCATION, "PRG test started");
    utils::Logger::InfoLog(LOCATION, "kPrgKeySeedLeft: " + fss::ToString(fss::prg::kPrgKeySeedLeft));
    utils::Logger::InfoLog(LOCATION, "kPrgKeySeedRight: " + fss::ToString(fss::prg::kPrgKeySeedRight));

    fss::prg::PseudoRandomGenerator *prg = new fss::prg::PseudoRandomGenerator(fss::prg::kPrgKeySeedLeft, fss::prg::kPrgKeySeedRight);
    utils::Logger::InfoLog(LOCATION, "PseudoRandomGenerator created");

    block                seed_in = makeBlock(0x1234567890abcdef, 0x1234567890abcdef);
    std::array<block, 2> seed_out;
    prg->DoubleExpand(seed_in, seed_out);
    utils::Logger::InfoLog(LOCATION, "DoubleExpand(seed_in, seed_out) called");

    utils::Logger::InfoLog(LOCATION, "seed_in: " + fss::ToString(seed_in));
    utils::Logger::InfoLog(LOCATION, "seed_out[0]: " + fss::ToString(seed_out[0]));
    utils::Logger::InfoLog(LOCATION, "seed_out[1]: " + fss::ToString(seed_out[1]));

    block zero_block = makeBlock(0, 0);
    block one_block  = makeBlock(1, 0);
    utils::Logger::InfoLog(LOCATION, "Equal test: zero_block == one_block: " + std::to_string(fss::Equal(zero_block, one_block)));
    utils::Logger::InfoLog(LOCATION, "Equal test: zero_block == zero_block: " + std::to_string(fss::Equal(zero_block, zero_block)));

    uint32_t              repeat = 1 << 20;
    utils::ExecutionTimer timer;
    timer.SetTimeUnit(utils::TimeUnit::NANOSECONDS);
    timer.Start();
    for (uint32_t i = 0; i < repeat; i++) {
        prg->DoubleExpand(seed_in, seed_out);
    }
    timer.Print(LOCATION, "DoubleExpand(seed_in, seed_out) elapsed time: ");
}

}    // namespace test
}    // namespace fsswm
