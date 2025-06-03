#ifndef BENCH_OBLIV_SELECT_BENCH_H_
#define BENCH_OBLIV_SELECT_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_fsswm {

void OblivSelect_Offline_Bench();
void OblivSelect_Online_Bench(const osuCrypto::CLP &cmd);
void OblivSelect_DotProduct_Bench();
void OblivSelect_Offline2_Bench();
void OblivSelect_Online2_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_fsswm

#endif    // BENCH_OBLIV_SELECT_BENCH_H_
