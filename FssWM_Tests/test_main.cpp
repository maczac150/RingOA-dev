#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/TestCollection.h>
#include <random>
#include <tests_cryptoTools/UnitTests.h>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM_Tests/fm_index/fssfmi_test.h"
#include "FssWM_Tests/fm_index/zt_test.h"
#include "FssWM_Tests/fss/dpf_test.h"
#include "FssWM_Tests/fss/prg_test.h"
#include "FssWM_Tests/sharing/additive_2p_test.h"
#include "FssWM_Tests/sharing/additive_3p_test.h"
#include "FssWM_Tests/sharing/binary_2p_test.h"
#include "FssWM_Tests/sharing/binary_3p_test.h"
#include "FssWM_Tests/utils/file_io_test.h"
#include "FssWM_Tests/utils/network_test.h"
#include "FssWM_Tests/utils/timer_test.h"
#include "FssWM_Tests/utils/utils_test.h"
#include "FssWM_Tests/wm/fsswm_test.h"
#include "FssWM_Tests/wm/obliv_select_test.h"
#include "FssWM_Tests/wm/wm_test.h"

namespace test_fsswm {

osuCrypto::TestCollection Tests([](osuCrypto::TestCollection &t) {
    t.add("Utils_Test", Utils_Test);
    t.add("Timer_Test", Timer_Test);
    t.add("Network_TwoPartyManager_Test", Network_TwoPartyManager_Test);
    t.add("Network_ThreePartyManager_Test", Network_ThreePartyManager_Test);
    t.add("File_Io_Test", File_Io_Test);
    t.add("Prg_Test", Prg_Test);
    t.add("Dpf_Params_Test", Dpf_Params_Test);
    t.add("Dpf_EvalAt_Test", Dpf_EvalAt_Test);
    t.add("Dpf_Fde_Test", Dpf_Fde_Test);
    t.add("Dpf_Fde_One_Test", Dpf_Fde_One_Test);
    t.add("Dpf_Pir_Test", Dpf_Pir_Test);
    t.add("Additive2P_EvaluateAdd_Offline_Test", Additive2P_EvaluateAdd_Offline_Test);
    t.add("Additive2P_EvaluateAdd_Online_Test", Additive2P_EvaluateAdd_Online_Test);
    t.add("Additive2P_EvaluateMult_Offline_Test", Additive2P_EvaluateMult_Offline_Test);
    t.add("Additive2P_EvaluateMult_Online_Test", Additive2P_EvaluateMult_Online_Test);
    t.add("Additive2P_EvaluateSelect_Offline_Test", Additive2P_EvaluateSelect_Offline_Test);
    t.add("Additive2P_EvaluateSelect_Online_Test", Additive2P_EvaluateSelect_Online_Test);
    t.add("Binary2P_EvaluateXor_Offline_Test", Binary2P_EvaluateXor_Offline_Test);
    t.add("Binary2P_EvaluateXor_Online_Test", Binary2P_EvaluateXor_Online_Test);
    t.add("Binary2P_EvaluateAnd_Offline_Test", Binary2P_EvaluateAnd_Offline_Test);
    t.add("Binary2P_EvaluateAnd_Online_Test", Binary2P_EvaluateAnd_Online_Test);
    t.add("Additive3P_Offline_Test", Additive3P_Offline_Test);
    t.add("Additive3P_Open_Online_Test", Additive3P_Open_Online_Test);
    t.add("Additive3P_EvaluateAdd_Online_Test", Additive3P_EvaluateAdd_Online_Test);
    t.add("Additive3P_EvaluateMult_Online_Test", Additive3P_EvaluateMult_Online_Test);
    t.add("Additive3P_EvaluateInnerProduct_Online_Test", Additive3P_EvaluateInnerProduct_Online_Test);
    t.add("Binary3P_Offline_Test", Binary3P_Offline_Test);
    t.add("Binary3P_Open_Online_Test", Binary3P_Open_Online_Test);
    t.add("Binary3P_EvaluateXor_Online_Test", Binary3P_EvaluateXor_Online_Test);
    t.add("Binary3P_EvaluateAnd_Online_Test", Binary3P_EvaluateAnd_Online_Test);
    t.add("Binary3P_EvaluateSelect_Online_Test", Binary3P_EvaluateSelect_Online_Test);
    t.add("OblivSelect_Offline_Test", OblivSelect_Offline_Test);
    t.add("OblivSelect_SingleBitMask_Online_Test", OblivSelect_SingleBitMask_Online_Test);
    t.add("OblivSelect_ShiftedAdditive_Online_Test", OblivSelect_ShiftedAdditive_Online_Test);
    t.add("WaveletMatrix_Test", WaveletMatrix_Test);
    t.add("FMIndex_Test", FMIndex_Test);
    t.add("ZeroTest_Binary_Offline_Test", ZeroTest_Binary_Offline_Test);
    t.add("ZeroTest_Binary_Online_Test", ZeroTest_Binary_Online_Test);
    t.add("FssWM_Offline_Test", FssWM_Offline_Test);
    t.add("FssWM_SingleBitMask_Online_Test", FssWM_SingleBitMask_Online_Test);
    t.add("FssWM_ShiftedAdditive_Online_Test", FssWM_ShiftedAdditive_Online_Test);
    t.add("FssFMI_Offline_Test", FssFMI_Offline_Test);
    t.add("FssFMI_SingleBitMask_Online_Test", FssFMI_SingleBitMask_Online_Test);
    t.add("FssFMI_ShiftedAdditive_Online_Test", FssFMI_ShiftedAdditive_Online_Test);
});

}    // namespace test_fsswm

namespace {

std::vector<std::string>
    helpTags{"h", "help"},
    listTags{"l", "list"},
    testTags{"t", "test"},
    unitTags{"u", "unitTests"},
    repeatTags{"repeat"},
    loopTags{"loop"};

void PrintHelp() {
    std::cout << "Usage: test_program [OPTIONS]\n";
    std::cout << "Options:\n";
    std::cout << "  -unit, -u           Run all unit tests.\n";
    std::cout << "  -list, -l           List all available tests.\n";
    std::cout << "  -test=<Index>, -t   Run the specified test by its index.\n";
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
        auto           tests = test_fsswm::Tests;

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
            auto testIdxs    = cmd.getMany<osuCrypto::u64>(testTags);
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

        // Unit test execution
        if (cmd.isSet(unitTags)) {
            fsswm::Logger::SetPrintLog(false);
            auto result = tests.runIf(cmd);
            if (result != osuCrypto::TestCollection::Result::passed) {
                return 1;    // Exit on failure
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
