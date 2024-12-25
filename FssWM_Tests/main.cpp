#include "cryptoTools/Common/CLP.h"
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/TestCollection.h"
#include "tests_cryptoTools/UnitTests.h"

#include "dpf_test.hpp"
#include "prg_test.hpp"

namespace test_fsswm {

oc::TestCollection Tests([](oc::TestCollection &t) {
    // t.add("Prg_Test", Prg_Test);
    t.add("Dpf_Params_Test", Dpf_Params_Test);
    t.add("Dpf_Fde_Test", Dpf_Fde_Test);
});

}    // namespace test_fsswm

int main(int argc, char **argv) {
    oc::CLP cmd(argc, argv);

    auto tests = test_fsswm::Tests;

    std::vector<uint64_t> testIdxs = {1};
    tests.run(testIdxs, 1);

    // tests.runIf(cmd);

    return 0;
}
