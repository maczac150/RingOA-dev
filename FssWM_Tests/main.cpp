#include "cryptoTools/Common/CLP.h"
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"
#include "tests_cryptoTools/UnitTests.h"

#include "dpf_test.hpp"
#include "prg_test.hpp"
#include "timer_test.hpp"

namespace test_fsswm {

oc::TestCollection Tests([](oc::TestCollection &t) {
    t.add("Timer_Test", Timer_Test);
    t.add("Prg_Test", Prg_Test);
    t.add("Dpf_Params_Test", Dpf_Params_Test);
    t.add("Dpf_Fde_Type_Test", Dpf_Fde_Type_Test);
    t.add("Dpf_Fde_Test", Dpf_Fde_Test);
    t.add("Dpf_Fde_One_Test", Dpf_Fde_One_Test);
    t.add("Dpf_Fde_Bench_Test", Dpf_Fde_Bench_Test);
    t.add("Dpf_Fde_One_Bench_Test", Dpf_Fde_One_Bench_Test);
});

}    // namespace test_fsswm

namespace {

std::vector<std::string>
    helpTags{"h", "help"},
    listTags{"l", "list"},
    testTags{"t", "test"},
    repeatTags{"repeat"},
    loopTags{"loop"};

void PrintHelp() {
    std::cout << "Usage: test_program [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  -list, -l           List all available tests with their indexes.\n";
    std::cout << "  -test=<Index>, -t   Run the specified test by its index.\n";
    std::cout << "  -repeat=<Count>     Specify the number of repetitions for the test (default: 1).\n";
    std::cout << "  -loop=<Count>       Repeat the entire test execution for the specified number of loops (default: 1).\n";
    std::cout << "  -help, -h           Display this help message.\n";
}

}    // namespace

int main(int argc, char **argv) {
    try {
        oc::CLP cmd(argc, argv);

        auto tests = test_fsswm::Tests;

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
        if (cmd.hasValue(testTags)) {
            auto testIdxs    = cmd.getMany<oc::u64>(testTags);
            int  repeatCount = cmd.getOr(repeatTags, 1);
            int  loopCount   = cmd.getOr(loopTags, 1);

            if (testIdxs.empty()) {
                std::cerr << "Error: No test index specified.\n";
                return 1;
            }

            // Execute tests in a loop
            for (int i = 0; i < loopCount; ++i) {
                auto result = tests.run(testIdxs, repeatCount);
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
