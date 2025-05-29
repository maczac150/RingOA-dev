#ifndef TESTS_OBLIV_SELECT_TEST_H_
#define TESTS_OBLIV_SELECT_TEST_H_

#include <cryptoTools/Common/CLP.h>

namespace test_fsswm {

void OblivSelect_Binary_Offline_Test();
void OblivSelect_Binary_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_OBLIV_SELECT_TEST_H_
