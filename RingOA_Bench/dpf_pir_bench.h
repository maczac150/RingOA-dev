#ifndef BENCH_DPF_PIR_BENCH_H_
#define BENCH_DPF_PIR_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void DpfPir_Offline_Bench(const osuCrypto::CLP &cmd);
void DpfPir_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_DPF_PIR_BENCH_H_
