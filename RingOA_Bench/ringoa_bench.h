#ifndef BENCH_RINGOA_BENCH_H_
#define BENCH_RINGOA_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void RingOa_Offline_Bench(const osuCrypto::CLP &cmd);
void RingOa_Online_Bench(const osuCrypto::CLP &cmd);
void RingOa_Fsc_Offline_Bench(const osuCrypto::CLP &cmd);
void RingOa_Fsc_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_RINGOA_BENCH_H_
