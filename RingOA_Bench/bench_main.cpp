#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/TestCollection.h>
#include <random>
#include <tests_cryptoTools/UnitTests.h>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA_Bench/dpf_bench.h"
#include "RingOA_Bench/dpf_pir_bench.h"
#include "RingOA_Bench/obliv_access_bench.h"
#include "RingOA_Bench/obliv_select_bench.h"
#include "RingOA_Bench/ofmi_bench.h"

namespace bench_ringoa {

osuCrypto::TestCollection Tests([](osuCrypto::TestCollection &t) {
    t.add("Dpf_Fde_Bench", Dpf_Fde_Bench);
    t.add("Dpf_Fde_Convert_Bench", Dpf_Fde_Convert_Bench);
    t.add("Dpf_Fde_One_Bench", Dpf_Fde_One_Bench);
    t.add("DpfPir_Offline_Bench", DpfPir_Offline_Bench);
    t.add("DpfPir_Online_Bench", DpfPir_Online_Bench);
    t.add("OblivSelect_ComputeDotProductBlockSIMD_Bench", OblivSelect_ComputeDotProductBlockSIMD_Bench);
    t.add("OblivSelect_EvaluateFullDomainThenDotProduct_Bench", OblivSelect_EvaluateFullDomainThenDotProduct_Bench);
    t.add("OblivSelect_Binary_Offline_Bench", OblivSelect_Binary_Offline_Bench);
    t.add("OblivSelect_Binary_Online_Bench", OblivSelect_Binary_Online_Bench);
    t.add("OblivSelect_Additive_Offline_Bench", OblivSelect_Additive_Offline_Bench);
    t.add("OblivSelect_Additive_Online_Bench", OblivSelect_Additive_Online_Bench);
    t.add("SharedOt_Offline_Bench", SharedOt_Offline_Bench);
    t.add("SharedOt_Online_Bench", SharedOt_Online_Bench);
    t.add("RingOa_Offline_Bench", RingOa_Offline_Bench);
    t.add("RingOa_Online_Bench", RingOa_Online_Bench);
    t.add("SotFMI_Offline_Bench", SotFMI_Offline_Bench);
    t.add("SotFMI_Online_Bench", SotFMI_Online_Bench);
    t.add("OFMI_Offline_Bench", OFMI_Offline_Bench);
    t.add("OFMI_Online_Bench", OFMI_Online_Bench);
});

}    // namespace bench_ringoa

namespace {

std::vector<std::string>
    helpTags{"h", "help"},
    listTags{"l", "list"},
    benchTags{"b", "bench"},
    repeatTags{"repeat"},
    loopTags{"loop"};

void PrintHelp() {
    std::cout << "Usage: bench_program [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  -list, -l           List all available benchmarks.\n";
    std::cout << "  -bench=<Index>, -b   Run the specified test by its index.\n";
    std::cout << "  -repeat=<Count>     Specify the number of repetitions for the test (default: 1).\n";
    std::cout << "  -loop=<Count>       Repeat the entire test execution for the specified number of loops (default: 1).\n";
    std::cout << "  -help, -h           Display this help message.\n";
}

}    // namespace

int main(int argc, char **argv) {
    try {
#ifndef USE_FIXED_RANDOM_SEED
        {
            std::random_device rd;
            osuCrypto::block   seed = osuCrypto::toBlock(rd(), rd());
            ringoa::GlobalRng::Initialize(seed);
        }
#else
        ringoa::GlobalRng::Initialize();
#endif
        osuCrypto::CLP cmd(argc, argv);
        auto           tests = bench_ringoa::Tests;

        // Display help message
        if (cmd.isSet(helpTags)) {
            PrintHelp();
            return 0;
        }

        // Display available tests
        if (cmd.isSet(listTags)) {
            tests.list();
            return 0;
        }

        // Handle test execution
        if (cmd.hasValue(benchTags)) {
            auto testIdxs    = cmd.getMany<osuCrypto::u64>(benchTags);
            int  repeatCount = cmd.getOr(repeatTags, 1);
            int  loopCount   = cmd.getOr(loopTags, 1);

            if (testIdxs.empty()) {
                std::cerr << "Error: No test index specified.\n";
                return 1;
            }

            // Execute tests in a loop
            for (int i = 0; i < loopCount; ++i) {
                auto result = tests.run(testIdxs, repeatCount, &cmd);
                if (result != osuCrypto::TestCollection::Result::passed) {
                    return 1;    // Exit on failure
                }
            }
            return 0;    // Success
        }

        // Invalid options
        std::cerr << "Error: No valid options specified.\n";
        PrintHelp();
        return 1;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
