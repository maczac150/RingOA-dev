#ifndef BENCH_FSSFMI_BENCH_H_
#define BENCH_FSSFMI_BENCH_H_

#include <cryptoTools/Common/CLP.h>

namespace bench_fsswm {

void SotFMI_Offline_Bench(const osuCrypto::CLP &cmd);
void SotFMI_Online_Bench(const osuCrypto::CLP &cmd);
void FssFMI_Offline_Bench(const osuCrypto::CLP &cmd);
void FssFMI_Online_Bench(const osuCrypto::CLP &cmd);

}    // namespace bench_fsswm

#endif    // BENCH_FSSFMI_BENCH_H_
