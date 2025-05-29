#ifndef TESTS_ZERO_TEST_TEST_H_
#define TESTS_ZERO_TEST_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_fsswm {

void ZeroTest_Binary_Offline_Test();
void ZeroTest_Binary_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_ZERO_TEST_TEST_H_
