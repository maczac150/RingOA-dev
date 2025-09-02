#ifndef BENCH_OFMI_BENCH_H_
#define BENCH_OFMI_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void SotFMI_Offline_Bench(const osuCrypto::CLP &cmd);
void SotFMI_Online_Bench(const osuCrypto::CLP &cmd);
void OFMI_Offline_Bench(const osuCrypto::CLP &cmd);
void OFMI_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_OFMI_BENCH_H_
