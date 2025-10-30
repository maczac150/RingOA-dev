#ifndef BENCH_OBLIV_ACCESS_BENCH_H_
#define BENCH_OBLIV_ACCESS_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void SharedOt_Offline_Bench(const osuCrypto::CLP &cmd);
void SharedOt_Online_Bench(const osuCrypto::CLP &cmd);
void RingOa_Offline_Bench(const osuCrypto::CLP &cmd);
void RingOa_Online_Bench(const osuCrypto::CLP &cmd);
void RingOa_Fsc_Offline_Bench(const osuCrypto::CLP &cmd);
void RingOa_Fsc_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_OBLIV_ACCESS_BENCH_H_
