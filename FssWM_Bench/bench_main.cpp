#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/TestCollection.h>
#include <random>
#include <tests_cryptoTools/UnitTests.h>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM_Bench/dpf_bench.h"
#include "FssWM_Bench/fssfmi_bench.h"
#include "FssWM_Bench/obliv_select_bench.h"

namespace bench_fsswm {

osuCrypto::TestCollection Tests([](osuCrypto::TestCollection &t) {
    t.add("Dpf_Fde_Bench", Dpf_Fde_Bench);
    t.add("Dpf_Fde_One_Bench", Dpf_Fde_One_Bench);
    t.add("Dpf_Pir_Bench", Dpf_Pir_Bench);
    t.add("OblivSelect_Offline_Bench", OblivSelect_Offline_Bench);
    t.add("OblivSelect_Online_Bench", OblivSelect_Online_Bench);
    t.add("OblivSelect_DotProduct_Bench", OblivSelect_DotProduct_Bench);
    t.add("FssFMI_Offline_Bench", FssFMI_Offline_Bench);
    t.add("FssFMI_Online_Bench", FssFMI_Online_Bench);
});

}    // namespace bench_fsswm

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
            fsswm::GlobalRng::Initialize(seed);
        }
#else
        fsswm::GlobalRng::Initialize();
#endif
        osuCrypto::CLP cmd(argc, argv);
        auto           tests = bench_fsswm::Tests;

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
