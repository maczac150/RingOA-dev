#ifndef FSS_PRG_H_
#define FSS_PRG_H_

#include "fss.hpp"

#include <array>

namespace fsswm {
namespace fss {
namespace prg {

const block kPrgKeySeedLeft  = makeBlock(0xf2416bf54f02e446, 0xcc2ce93fdbcccc28);    // First half of the SHA256 hash for `dpf::kPrgKeySeedLeft`
const block kPrgKeySeedRight = makeBlock(0x65776b0991b8d225, 0xdac18583c2123349);    // First half of the SHA256 hash for `dpf::kPrgKeySeedRight`

/**
 * @brief Pseudo-random generator class.
 */
class PseudoRandomGenerator {
public:
    /**
     * @brief Default constructor for PseudoRandomGenerator.
     * @param init_seed0 The initial seed for the PRG.
     * @param init_seed1 The initial seed for the PRG.
     */
    PseudoRandomGenerator(block init_seed0, block init_seed1) {
        utils::Logger::DebugLog(LOC, "Initializing PRG", true);
        AES_set_encrypt_key((const block)init_seed0, aes_key);
        AES_set_encrypt_key((const block)init_seed1, &aes_key[1]);
    }

    /**
     * @brief Encrypts the input block using the PRG.
     * @param seed_in The input block to be encrypted.
     * @param seed_out The output block after encryption.
     * @note PRG: G(s) = PRF_seed0(s) ^ s || PRF_seed1(s) ^ s
     */
    void DoubleExpand(block seed_in, std::array<block, 2> &seed_out) {
        block tmp[2];
        tmp[0] = seed_out[0] = seed_in;
        tmp[1] = seed_out[1] = seed_in;
        ParaEnc<2, 1>(tmp, aes_key);
        seed_out[0] = seed_out[0] ^ tmp[0];
        seed_out[1] = seed_out[1] ^ tmp[1];
    }

    void DoubleExpand(std::array<block, 8> &seed_in, std::array<std::array<block, 2>, 8> &seed_out) {
        block tmp[8][2];
        for (uint32_t i = 0; i < 8; i++) {
            tmp[i][0] = seed_out[i][0] = seed_in[i];
            tmp[i][1] = seed_out[i][1] = seed_in[i];
            ParaEnc<2, 1>(tmp[i], aes_key);
            seed_out[i][0] = seed_out[i][0] ^ tmp[i][0];
            seed_out[i][1] = seed_out[i][1] ^ tmp[i][1];
        }
    }

private:
    AES_KEY aes_key[2]; /**< AES key for the PRG. */
};

class PseudoRandomGeneratorSingleton {
public:
    static PseudoRandomGenerator &GetInstance() {
        static PseudoRandomGenerator instance(kPrgKeySeedLeft, kPrgKeySeedRight);
        return instance;
    }

    PseudoRandomGeneratorSingleton(const PseudoRandomGeneratorSingleton &)            = delete;
    PseudoRandomGeneratorSingleton &operator=(const PseudoRandomGeneratorSingleton &) = delete;

private:
    PseudoRandomGeneratorSingleton() = default;
};

}    // namespace prg
}    // namespace fss
}    // namespace fsswm

#endif    // PRG_PRG_H_
