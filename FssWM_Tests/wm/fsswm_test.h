#ifndef TESTS_FSSWM_TEST_H_
#define TESTS_FSSWM_TEST_H_

#include "cryptoTools/Common/CLP.h"

namespace test_fsswm {

void FssWM_Offline_Test();
void FssWM_Online_Test(const oc::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_FSSWM_TEST_H_
