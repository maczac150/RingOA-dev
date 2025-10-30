#ifndef BENCH_DPF_BENCH_H_
#define BENCH_DPF_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void Dpf_Fde_Bench(const osuCrypto::CLP &cmd);
void Dpf_Fde_Convert_Bench(const osuCrypto::CLP &cmd);
void Dpf_Fde_One_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_DPF_BENCH_H_
