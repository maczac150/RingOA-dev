#ifndef BENCH_SECURE_FMI_BENCH_H_
#define BENCH_SECURE_FMI_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_ringoa {

void SotFMI_Offline_Bench(const osuCrypto::CLP &cmd);
void SotFMI_Online_Bench(const osuCrypto::CLP &cmd);
void SecureFMI_Offline_Bench(const osuCrypto::CLP &cmd);
void SecureFMI_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_ringoa

#endif    // BENCH_SECURE_FMI_BENCH_H_
