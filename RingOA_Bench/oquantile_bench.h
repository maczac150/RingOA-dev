#ifndef BENCH_OQUANTILE_BENCH_H_
#define BENCH_OQUANTILE_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void OQuantile_Offline_Bench(const osuCrypto::CLP &cmd);
void OQuantile_Online_Bench(const osuCrypto::CLP &cmd);
void OQuantile_VAF_Offline_Bench(const osuCrypto::CLP &cmd);
void OQuantile_VAF_Online_Bench(const osuCrypto::CLP &cmd);
void OQuantile_Fsc_VAF_Offline_Bench(const osuCrypto::CLP &cmd);
void OQuantile_Fsc_VAF_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_OQUANTILE_BENCH_H_
