#ifndef FSS_PRG_H_
#define FSS_PRG_H_

#include "fss.hpp"

#include <array>

namespace fsswm {
namespace fss {
namespace prg {

const block kPrgKeySeedLeft  = makeBlock(0xf2416bf54f02e446, 0xcc2ce93fdbcccc28);    // First half of the SHA256 hash for `dpf::kPrgKeySeedLeft`
const block kPrgKeySeedRight = makeBlock(0x65776b0991b8d225, 0xdac18583c2123349);    // First half of the SHA256 hash for `dpf::kPrgKeySeedRight`

class PseudoRandomGenerator {
public:
    PseudoRandomGenerator(block init_seed0, block init_seed1) {
        AES_set_encrypt_key((const block)init_seed0, aes_key);
        AES_set_encrypt_key((const block)init_seed1, &aes_key[1]);
    }

    // PRG: G(s) = PRF_seed0(s) ^ s || PRF_seed1(s) ^ s
    void DoubleExpand(block seed_in, std::array<block, 2> &seed_out) {
        block tmp[2];
        tmp[0] = seed_out[0] = seed_in;
        tmp[1] = seed_out[1] = seed_in;
        ParaEnc<2, 1>(tmp, aes_key);
        seed_out[0] = seed_out[0] ^ tmp[0];
        seed_out[1] = seed_out[1] ^ tmp[1];
    }

private:
    AES_KEY aes_key[2];
};

}    // namespace prg
}    // namespace fss
}    // namespace fsswm

#endif    // PRG_PRG_H_
