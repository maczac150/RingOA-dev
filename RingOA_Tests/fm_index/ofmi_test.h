#ifndef TESTS_OFMI_TEST_H_
#define TESTS_OFMI_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void SotFMI_Offline_Test();
void SotFMI_Online_Test(const osuCrypto::CLP &cmd);
void OFMI_Offline_Test();
void OFMI_Online_Test(const osuCrypto::CLP &cmd);
void OFMI_Fsc_Offline_Test();
void OFMI_Fsc_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_OFMI_TEST_H_
