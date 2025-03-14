#ifndef TESTS_ZERO_TEST_TEST_H_
#define TESTS_ZERO_TEST_TEST_H_

#include "cryptoTools/Common/CLP.h"

namespace test_fsswm {

void ZeroTest_Additive_Offline_Test();
void ZeroTest_Additive_Online_Test(const oc::CLP &cmd);
void ZeroTest_Binary_Offline_Test();
void ZeroTest_Binary_Online_Test(const oc::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_ZERO_TEST_TEST_H_
