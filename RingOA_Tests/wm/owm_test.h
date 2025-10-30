#ifndef TESTS_OWM_TEST_H_
#define TESTS_OWM_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void OWM_Offline_Test();
void OWM_Online_Test(const osuCrypto::CLP &cmd);
void OWM_Fsc_Offline_Test();
void OWM_Fsc_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_OWM_TEST_H_
