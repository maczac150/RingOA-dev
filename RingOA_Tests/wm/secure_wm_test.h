#ifndef TESTS_SECURE_WM_TEST_H_
#define TESTS_SECURE_WM_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void SecureWM_Offline_Test();
void SecureWM_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_SECURE_WM_TEST_H_
