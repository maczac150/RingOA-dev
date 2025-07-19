#ifndef TESTS_INTEGER_COMPARISON_H_
#define TESTS_INTEGER_COMPARISON_H_

#include <cryptoTools/Common/CLP.h>

namespace test_fsswm {

void IntegerComparison_Offline_Test();
void IntegerComparison_Online_Test(const osuCrypto::CLP &cmd);

}    // namespace test_fsswm

#endif    // TESTS_INTEGER_COMPARISON_H_
