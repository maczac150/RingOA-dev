#ifndef TESTS_RINGOA_TEST_H_
#define TESTS_RINGOA_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void RingOa_Offline_Test();
void RingOa_Online_Test(const osuCrypto::CLP &cmd);
void RingOa_Fsc_Offline_Test();
void RingOa_Fsc_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_RINGOA_TEST_H_
