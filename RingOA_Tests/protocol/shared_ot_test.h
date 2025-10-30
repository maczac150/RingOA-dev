#ifndef TESTS_SHARED_OT_TEST_H_
#define TESTS_SHARED_OT_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_ringoa {

void SharedOt_Offline_Test();
void SharedOt_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_ringoa

#endif    // TESTS_SHARED_OT_TEST_H_
