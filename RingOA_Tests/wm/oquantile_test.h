#ifndef TESTS_OQUANTILE_TEST_H_
#define TESTS_OQUANTILE_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void OQuantile_Offline_Test();
void OQuantile_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_OQUANTILE_TEST_H_
