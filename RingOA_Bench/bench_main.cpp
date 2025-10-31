#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/TestCollection.h>
#include <random>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA_Bench/dpf_bench.h"
#include "RingOA_Bench/dpf_pir_bench.h"
#include "RingOA_Bench/obliv_select_bench.h"
#include "RingOA_Bench/ofmi_bench.h"
#include "RingOA_Bench/oquantile_bench.h"
#include "RingOA_Bench/ringoa_bench.h"
#include "RingOA_Bench/shared_ot_bench.h"
#include "RingOA_Bench/sotfmi_bench.h"

namespace bench_ringoa {

osuCrypto::TestCollection Tests([](osuCrypto::TestCollection &t) {
    t.add("Dpf_Fde_Bench", Dpf_Fde_Bench);
    t.add("Dpf_Fde_Convert_Bench", Dpf_Fde_Convert_Bench);
    t.add("Dpf_Fde_One_Bench", Dpf_Fde_One_Bench);
    t.add("DpfPir_Offline_Bench", DpfPir_Offline_Bench);
    t.add("DpfPir_Online_Bench", DpfPir_Online_Bench);

    t.add("RingOa_Offline_Bench", RingOa_Offline_Bench);
    t.add("RingOa_Online_Bench", RingOa_Online_Bench);
    t.add("RingOa_Fsc_Offline_Bench", RingOa_Fsc_Offline_Bench);
    t.add("RingOa_Fsc_Online_Bench", RingOa_Fsc_Online_Bench);

    t.add("OFMI_Offline_Bench", OFMI_Offline_Bench);
    t.add("OFMI_Online_Bench", OFMI_Online_Bench);
    t.add("OFMI_Fsc_Offline_Bench", OFMI_Fsc_Offline_Bench);
    t.add("OFMI_Fsc_Online_Bench", OFMI_Fsc_Online_Bench);

    t.add("OQuantile_Offline_Bench", OQuantile_Offline_Bench);
    t.add("OQuantile_Online_Bench", OQuantile_Online_Bench);

    t.add("SharedOt_Offline_Bench", SharedOt_Offline_Bench);
    t.add("SharedOt_Online_Bench", SharedOt_Online_Bench);
    t.add("SharedOt_Naive_Offline_Bench", SharedOt_Naive_Offline_Bench);
    t.add("SharedOt_Naive_Online_Bench", SharedOt_Naive_Online_Bench);
    t.add("SotFMI_Offline_Bench", SotFMI_Offline_Bench);
    t.add("SotFMI_Online_Bench", SotFMI_Online_Bench);

    // t.add("OblivSelect_ComputeDotProductBlockSIMD_Bench", OblivSelect_ComputeDotProductBlockSIMD_Bench);
    // t.add("OblivSelect_EvaluateFullDomainThenDotProduct_Bench", OblivSelect_EvaluateFullDomainThenDotProduct_Bench);
    // t.add("OblivSelect_SingleBitMask_Offline_Bench", OblivSelect_SingleBitMask_Offline_Bench);
    // t.add("OblivSelect_SingleBitMask_Online_Bench", OblivSelect_SingleBitMask_Online_Bench);
    // t.add("OblivSelect_ShiftedAdditive_Offline_Bench", OblivSelect_ShiftedAdditive_Offline_Bench);
    // t.add("OblivSelect_ShiftedAdditive_Online_Bench", OblivSelect_ShiftedAdditive_Online_Bench);
});

}    // namespace bench_ringoa

namespace {

std::vector<std::string>
    helpTags{"h", "help"},
    listTags{"l", "list"},
    benchTags{"b", "bench"},
    sizeTags{"s", "size"},
    repeatTags{"repeat"};

void PrintHelp(const char *prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  --list, -l            List all available benchmarks.\n";
    std::cout << "  --bench <Index>, -b   Run the specified test by its index.\n";
    std::cout << "  --size <Name>, -s     Benchmark scale: small, medium, large, even (default: full range).\n";
    std::cout << "  --repeat <Count>      Number of repetitions within each benchmark (default: 3).\n";
    std::cout << "  --help, -h            Display this help message.\n";
}

}    // namespace

int main(int argc, char **argv) {
    try {
#if !USE_FIXED_RANDOM_SEED
        {
            std::random_device rd;
            osuCrypto::block   seed = osuCrypto::toBlock(rd(), rd());
            ringoa::GlobalRng::Initialize(seed);
            std::cout << "[bench] RNG initialized with random seed " << seed << "\n";
        }
#else
        ringoa::GlobalRng::Initialize();
        std::cout << "[bench] RNG initialized with fixed default seed\n";
#endif
        osuCrypto::CLP cmd(argc, argv);
        auto          &tests = bench_ringoa::Tests;

        // Display help message
        if (cmd.isSet(helpTags)) {
            PrintHelp(argv[0]);
            return 0;
        }

        // Display available tests
        if (cmd.isSet(listTags)) {
            tests.list();
            return 0;
        }

        // Handle test execution
        if (cmd.hasValue(benchTags)) {
            auto testIdxs = cmd.getMany<osuCrypto::u64>(benchTags);
            if (testIdxs.empty()) {
                std::cerr << "Error: No test index specified.\n";
                return 1;
            }

            // validate repeat value
            int repeatCount = cmd.getOr(repeatTags, 3);
            if (repeatCount < 1) {
                std::cerr << "Error: repeat must be >= 1.\n";
                return 1;
            }

            auto result = tests.run(testIdxs, /*repeatCount=*/1, &cmd);
            return (result == osuCrypto::TestCollection::Result::passed) ? 0 : 1;
        }

        // Invalid options
        std::cerr << "Error: No valid options specified.\n";
        std::cerr << "       Use -list to see available tests or -help for usage.\n";
        PrintHelp(argv[0]);
        return 1;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
