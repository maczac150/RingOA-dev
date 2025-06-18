#ifndef TESTS_OBLIV_SELECT_TEST_H_
#define TESTS_OBLIV_SELECT_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_fsswm {

void OblivSelect_Offline_Test();
void OblivSelect_SingleBitMask_Online_Test(const osuCrypto::CLP &cmd);
void OblivSelect_ShiftedAdditive_Online_Test(const osuCrypto::CLP &cmd);
void MixedOblivSelect_Offline_Test();
void MixedOblivSelect_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_OBLIV_SELECT_TEST_H_
