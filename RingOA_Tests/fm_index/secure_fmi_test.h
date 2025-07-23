#ifndef TESTS_SECURE_FMI_TEST_H_
#define TESTS_SECURE_FMI_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void SotFMI_Offline_Test();
void SotFMI_Online_Test(const osuCrypto::CLP &cmd);
void SecureFMI_Offline_Test();
void SecureFMI_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_SECURE_FMI_TEST_H_
