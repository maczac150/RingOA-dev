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
     * @brief Encrypts the input block using a single AES key.
     * @param seed_in The input block to be encrypted.
     * @param seed_out The output block after encryption.
     * @param key_lr The AES key to be used.
     */
    void Expand(block seed_in, block &seed_out, bool key_lr) {
        block tmp = seed_in;
        AES_ecb_encrypt_blks(&tmp, 1, &aes_key[key_lr]);
        seed_out = seed_in ^ tmp;
    }

    /**
     * @brief Encrypts an array of 8 input blocks using a single AES key.
     * @param seed_in The input blocks to be encrypted.
     * @param seed_out The output blocks after encryption.
     * @param key_lr The AES key to be used.
     */
    void Expand(std::array<block, 8> &seed_in, std::array<block, 8> &seed_out, bool key_lr) {
        std::array<block, 8> tmp = seed_in;
        AES_ecb_encrypt_blks(tmp.data(), 8, &aes_key[key_lr]);
        seed_out[0] = seed_in[0] ^ tmp[0];
        seed_out[1] = seed_in[1] ^ tmp[1];
        seed_out[2] = seed_in[2] ^ tmp[2];
        seed_out[3] = seed_in[3] ^ tmp[3];
        seed_out[4] = seed_in[4] ^ tmp[4];
        seed_out[5] = seed_in[5] ^ tmp[5];
        seed_out[6] = seed_in[6] ^ tmp[6];
        seed_out[7] = seed_in[7] ^ tmp[7];
        // for (size_t i = 0; i < 8; ++i) {
        //     seed_out[i] = seed_in[i] ^ tmp[i];
        // }
    }

    /**
     * @brief Encrypts the input block using the PRG.
     * @param seed_in The input block to be encrypted.
     * @param seed_out The output block after encryption.
     */
    void DoubleExpand(block seed_in, std::array<block, 2> &seed_out) {
        block tmp[2];
        tmp[0] = seed_out[0] = seed_in;
        tmp[1] = seed_out[1] = seed_in;

        ParaEnc<2, 1>(tmp, aes_key);

        seed_out[0] = seed_out[0] ^ tmp[0];
        seed_out[1] = seed_out[1] ^ tmp[1];
    }

    /**
     * @brief Encrypts the input blocks using the PRG with separate keys.
     * @param seed_in The input blocks to be encrypted.
     * @param seed_out The output blocks after encryption with left and right keys.
     */
    void DoubleExpand(std::array<block, 8> &seed_in, std::array<std::array<block, 8>, 2> &seed_out) {
        for (uint32_t i = 0; i < 8; i++) {
            block tmp[2];
            tmp[0] = seed_out[0][i] = seed_in[i];
            tmp[1] = seed_out[1][i] = seed_in[i];

            ParaEnc<2, 1>(tmp, aes_key);

            seed_out[0][i] = seed_out[0][i] ^ tmp[0];
            seed_out[1][i] = seed_out[1][i] ^ tmp[1];
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
