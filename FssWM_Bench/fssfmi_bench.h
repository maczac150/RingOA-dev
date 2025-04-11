#ifndef BENCH_FSSFMI_BENCH_H_
#define BENCH_FSSFMI_BENCH_H_

#include "cryptoTools/Common/CLP.h"

namespace bench_fsswm {

void FssFMI_Offline_Bench();
void FssFMI_Online_Bench(const oc::CLP &cmd);

}    // namespace bench_fsswm

#endif    // BENCH_FSSFMI_BENCH_H_
