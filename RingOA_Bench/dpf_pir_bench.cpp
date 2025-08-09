#include "dpf_pir_bench.h"

#include <cryptoTools/Common/TestCollection.h>
#include <thread>

#include "RingOA/protocol/dpf_pir.h"
#include "RingOA/utils/file_io.h"
#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA/utils/timer.h"
#include "RingOA/utils/to_string.h"
#include "RingOA/utils/utils.h"

namespace bench_ringoa {

using ringoa::block;
using ringoa::FileIo;
using ringoa::GlobalRng;
using ringoa::Logger;
using ringoa::Mod;
using ringoa::TimerManager;
using ringoa::ToString;
using ringoa::fss::EvalType, ringoa::fss::OutputType;
using ringoa::proto::DpfPirEvaluator;
using ringoa::proto::DpfPirKey;
using ringoa::proto::DpfPirKeyGenerator;
using ringoa::proto::DpfPirParameters;

void DpfPir_Bench() {
    uint64_t              repeat = 50;
    std::vector<uint64_t> sizes  = {16, 18, 20, 22, 24, 26, 28};
    // std::vector<uint64_t> sizes      = ringoa::CreateSequence(10, 30);

    Logger::InfoLog(LOC, "DpfPir Benchmark started");
    for (auto size : sizes) {
    }
    Logger::InfoLog(LOC, "DpfPir Benchmark completed");
}

}    // namespace bench_ringoa
