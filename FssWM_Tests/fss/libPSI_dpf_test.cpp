#include "libPSI_dpf_test.h"

#include "FssWM/fss/libPSI_dpf_eval.h"
#include "FssWM/fss/libPSI_dpf_gen.h"
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/TestCollection.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Network/IOService.h>

namespace test_fsswm {

using namespace osuCrypto;

void libPSI_DPF_FullDomain_Test() {
    std::cout << "libPSI_DPF_FullDomain_Test" << std::endl;
    // std::vector<std::array<u64, 2>> params{{9, 1}, {13, 1}, {17, 1}, {21, 1}};
    std::vector<std::array<u64, 2>> params{{3, 1}};

    for (auto param : params) {

        u64 depth = param[0], groupBlkSize = param[1];
        u64 domain = (1ull << depth) * groupBlkSize * 128;
        u64 trials = 10;

        std::vector<block> data(domain);
        for (u64 i = 0; i < data.size(); ++i)
            data[i] = toBlock(i);

        std::vector<block> k0(depth + 1), k1(depth + 1);
        std::vector<block> g0(groupBlkSize), g1(groupBlkSize);

        PRNG             prng(ZeroBlock);
        std::vector<u64> durations;
        for (u64 i = 0; i < trials; ++i) {
            auto idx = (prng.get<int>()) % domain;
            BgiPirClient::keyGen(idx, toBlock(idx), k0, g0, k1, g1);

            auto start    = std::chrono::high_resolution_clock::now();
            auto b0       = BgiPirServer::fullDomain(data, k0, g0);
            auto end      = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            durations.push_back(duration);
            auto b1 = BgiPirServer::fullDomain(data, k1, g1);

            if (neq(b0 ^ b1, data[idx])) {
                // auto vv = bv0 ^ bv1;
                std::cout << "target " << data[idx] << " " << idx << "\n  " << (b0 ^ b1) << "\n = " << b0 << " ^ " << b1 <<
                    //"\n   weight " << vv.hammingWeight() <<
                    //"\n vv[target] = " << vv[idx] <<
                    std::endl;
                throw std::runtime_error(LOCATION);
            }
        }
        u64 totalDuration   = std::accumulate(durations.begin(), durations.end(), 0);
        u64 averageDuration = totalDuration / durations.size();
        std::cout << "Average n=" << param[0] + 7 << " (" << trials << " trials) " << averageDuration << " us" << std::endl;
    }
}

void libPSI_DPF_FullDomain2_Test() {
    std::cout << "libPSI_DPF_FullDomain_Test" << std::endl;
    std::vector<u64> params{9, 13, 17, 21};
    // std::vector<u64> params{3};

    for (auto param : params) {
        u64 depth  = param;
        u64 domain = (1ull << depth) * 128;
        u64 trials = 10;

        std::vector<block> data(domain);
        for (u64 i = 0; i < data.size(); ++i)
            data[i] = toBlock(i);

        std::vector<block> k0(depth + 1), k1(depth + 1);
        std::vector<block> g0(1), g1(1);

        PRNG             prng(ZeroBlock);
        std::vector<u64> durations;
        for (u64 i = 0; i < trials; ++i) {
            auto idx = (prng.get<int>()) % domain;
            BgiPirClient::keyGenSingleGroup(idx, toBlock(idx), k0, g0, k1, g1);

            auto start    = std::chrono::high_resolution_clock::now();
            auto b0       = BgiPirServer::fullDomainSingleGroup(data, k0, g0);
            auto end      = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            durations.push_back(duration);
            auto b1 = BgiPirServer::fullDomainSingleGroup(data, k1, g1);

            if (neq(b0 ^ b1, data[idx])) {
                // auto vv = bv0 ^ bv1;
                std::cout << "target " << data[idx] << " " << idx << "\n  " << (b0 ^ b1) << "\n = " << b0 << " ^ " << b1 <<
                    //"\n   weight " << vv.hammingWeight() <<
                    //"\n vv[target] = " << vv[idx] <<
                    std::endl;
                throw std::runtime_error(LOCATION);
            }
        }
        u64 totalDuration   = std::accumulate(durations.begin(), durations.end(), 0);
        u64 averageDuration = totalDuration / durations.size();
        std::cout << "Average n=" << param + 7 << " (" << trials << " trials) " << averageDuration << " us" << std::endl;
    }

}

}    // namespace test_fsswm
