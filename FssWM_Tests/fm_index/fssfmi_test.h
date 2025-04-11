#ifndef TESTS_FSSFMI_TEST_H_
#define TESTS_FSSFMI_TEST_H_

#include "cryptoTools/Common/CLP.h"

namespace test_fsswm {

void FssFMI_Offline_Test();
void FssFMI_Online_Test(const oc::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_FSSFMI_TEST_H_
