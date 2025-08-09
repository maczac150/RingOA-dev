#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/TestCollection.h>
#include <random>
#include <tests_cryptoTools/UnitTests.h>

#include "RingOA/utils/logger.h"
#include "RingOA/utils/rng.h"
#include "RingOA_Tests/fm_index/secure_fmi_test.h"
#include "RingOA_Tests/fss/dcf_test.h"
#include "RingOA_Tests/fss/dpf_test.h"
#include "RingOA_Tests/fss/prg_test.h"
#include "RingOA_Tests/protocol/ddcf_test.h"
#include "RingOA_Tests/protocol/dpf_pir_test.h"
#include "RingOA_Tests/protocol/equality_test.h"
#include "RingOA_Tests/protocol/integer_comparison_test.h"
#include "RingOA_Tests/protocol/obliv_access_test.h"
#include "RingOA_Tests/protocol/zt_test.h"
#include "RingOA_Tests/sharing/additive_2p_test.h"
#include "RingOA_Tests/sharing/additive_3p_test.h"
#include "RingOA_Tests/sharing/binary_2p_test.h"
#include "RingOA_Tests/sharing/binary_3p_test.h"
#include "RingOA_Tests/utils/file_io_test.h"
#include "RingOA_Tests/utils/network_test.h"
#include "RingOA_Tests/utils/timer_test.h"
#include "RingOA_Tests/utils/utils_test.h"
#include "RingOA_Tests/wm/secure_wm_test.h"
#include "RingOA_Tests/wm/wm_test.h"

namespace test_ringoa {

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
    t.add("Dcf_EvalAt_Test", Dcf_EvalAt_Test);
    t.add("Dcf_Fde_Test", Dcf_Fde_Test);
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
    t.add("Ddcf_EvalAt_Test", Ddcf_EvalAt_Test);
    t.add("Ddcf_Fde_Test", Ddcf_Fde_Test);
    t.add("ZeroTest_Offline_Test", ZeroTest_Offline_Test);
    t.add("ZeroTest_Online_Test", ZeroTest_Online_Test);
    t.add("Equality_Offline_Test", Equality_Offline_Test);
    t.add("Equality_Online_Test", Equality_Online_Test);
    t.add("IntegerComparison_Offline_Test", IntegerComparison_Offline_Test);
    t.add("IntegerComparison_Online_Test", IntegerComparison_Online_Test);
    t.add("DpfPir_Offline_Test", DpfPir_Offline_Test);
    t.add("DpfPir_Online_Test", DpfPir_Online_Test);
    t.add("OblivSelect_Offline_Test", OblivSelect_Offline_Test);
    t.add("OblivSelect_SingleBitMask_Online_Test", OblivSelect_SingleBitMask_Online_Test);
    t.add("OblivSelect_ShiftedAdditive_Online_Test", OblivSelect_ShiftedAdditive_Online_Test);
    t.add("SharedOt_Offline_Test", SharedOt_Offline_Test);
    t.add("SharedOt_Online_Test", SharedOt_Online_Test);
    t.add("RingOa_Offline_Test", RingOa_Offline_Test);
    t.add("RingOa_Online_Test", RingOa_Online_Test);
    t.add("WaveletMatrix_Test", WaveletMatrix_Test);
    t.add("FMIndex_Test", FMIndex_Test);
    t.add("SecureWM_Offline_Test", SecureWM_Offline_Test);
    t.add("SecureWM_Online_Test", SecureWM_Online_Test);
    t.add("SotFMI_Offline_Test", SotFMI_Offline_Test);
    t.add("SotFMI_Online_Test", SotFMI_Online_Test);
    t.add("SecureFMI_Offline_Test", SecureFMI_Offline_Test);
    t.add("SecureFMI_Online_Test", SecureFMI_Online_Test);
});

}    // namespace test_ringoa

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
            ringoa::GlobalRng::Initialize(seed);
        }
#else
        ringoa::GlobalRng::Initialize();
#endif

        osuCrypto::CLP cmd(argc, argv);
        auto           tests = test_ringoa::Tests;

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
            ringoa::Logger::SetPrintLog(false);
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
