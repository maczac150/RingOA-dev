#ifndef BENCH_OBLIV_SELECT_BENCH_H_
#define BENCH_OBLIV_SELECT_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_fsswm {

void OblivSelect_ComputeDotProductBlockSIMD_Bench(const osuCrypto::CLP &cmd);
void OblivSelect_EvaluateFullDomainThenDotProduct_Bench(const osuCrypto::CLP &cmd);
void OblivSelect_Binary_Offline_Bench(const osuCrypto::CLP &cmd);
void OblivSelect_Binary_Online_Bench(const osuCrypto::CLP &cmd);
void OblivSelect_Additive_Offline_Bench(const osuCrypto::CLP &cmd);
void OblivSelect_Additive_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_fsswm

#endif    // BENCH_OBLIV_SELECT_BENCH_H_
