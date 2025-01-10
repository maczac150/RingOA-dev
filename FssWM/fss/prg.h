#ifndef FSS_PRG_H_
#define FSS_PRG_H_

#include <emp-tool/utils/aes.h>

#include "fss.h"

namespace fsswm {
namespace fss {
namespace prg {

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
    PseudoRandomGenerator(block init_seed0, block init_seed1);

    /**
     * @brief Encrypts the input block using a single AES key.
     * @param seed_in The input block to be encrypted.
     * @param seed_out The output block after encryption.
     * @param key_lr The AES key to be used.
     */
    void Expand(block seed_in, block &seed_out, bool key_lr);

    /**
     * @brief Encrypts an array of 8 input blocks using a single AES key.
     * @param seed_in The input blocks to be encrypted.
     * @param seed_out The output blocks after encryption.
     * @param key_lr The AES key to be used.
     */
    inline void Expand(std::array<block, 8> &seed_in, std::array<block, 8> &seed_out, bool key_lr) {
        std::array<block, 8> tmp;

        tmp[0] = seed_in[0];
        tmp[1] = seed_in[1];
        tmp[2] = seed_in[2];
        tmp[3] = seed_in[3];
        tmp[4] = seed_in[4];
        tmp[5] = seed_in[5];
        tmp[6] = seed_in[6];
        tmp[7] = seed_in[7];

        emp::AES_ecb_encrypt_blks(tmp.data(), 8, &aes_key[key_lr]);

        seed_out[0] = _mm_xor_si128(seed_in[0], tmp[0]);
        seed_out[1] = _mm_xor_si128(seed_in[1], tmp[1]);
        seed_out[2] = _mm_xor_si128(seed_in[2], tmp[2]);
        seed_out[3] = _mm_xor_si128(seed_in[3], tmp[3]);
        seed_out[4] = _mm_xor_si128(seed_in[4], tmp[4]);
        seed_out[5] = _mm_xor_si128(seed_in[5], tmp[5]);
        seed_out[6] = _mm_xor_si128(seed_in[6], tmp[6]);
        seed_out[7] = _mm_xor_si128(seed_in[7], tmp[7]);
    }

    inline void Expand(std::array<block, 16> &seed_in, std::array<block, 16> &seed_out, bool key_lr) {
        std::array<block, 16> tmp;

        tmp[0]  = seed_in[0];
        tmp[1]  = seed_in[1];
        tmp[2]  = seed_in[2];
        tmp[3]  = seed_in[3];
        tmp[4]  = seed_in[4];
        tmp[5]  = seed_in[5];
        tmp[6]  = seed_in[6];
        tmp[7]  = seed_in[7];
        tmp[8]  = seed_in[8];
        tmp[9]  = seed_in[9];
        tmp[10] = seed_in[10];
        tmp[11] = seed_in[11];
        tmp[12] = seed_in[12];
        tmp[13] = seed_in[13];
        tmp[14] = seed_in[14];
        tmp[15] = seed_in[15];

        emp::AES_ecb_encrypt_blks(tmp.data(), 16, &aes_key[key_lr]);

        seed_out[0]  = _mm_xor_si128(seed_in[0], tmp[0]);
        seed_out[1]  = _mm_xor_si128(seed_in[1], tmp[1]);
        seed_out[2]  = _mm_xor_si128(seed_in[2], tmp[2]);
        seed_out[3]  = _mm_xor_si128(seed_in[3], tmp[3]);
        seed_out[4]  = _mm_xor_si128(seed_in[4], tmp[4]);
        seed_out[5]  = _mm_xor_si128(seed_in[5], tmp[5]);
        seed_out[6]  = _mm_xor_si128(seed_in[6], tmp[6]);
        seed_out[7]  = _mm_xor_si128(seed_in[7], tmp[7]);
        seed_out[8]  = _mm_xor_si128(seed_in[8], tmp[8]);
        seed_out[9]  = _mm_xor_si128(seed_in[9], tmp[9]);
        seed_out[10] = _mm_xor_si128(seed_in[10], tmp[10]);
        seed_out[11] = _mm_xor_si128(seed_in[11], tmp[11]);
        seed_out[12] = _mm_xor_si128(seed_in[12], tmp[12]);
        seed_out[13] = _mm_xor_si128(seed_in[13], tmp[13]);
        seed_out[14] = _mm_xor_si128(seed_in[14], tmp[14]);
        seed_out[15] = _mm_xor_si128(seed_in[15], tmp[15]);
    }

    /**
     * @brief Encrypts the input block using the PRG.
     * @param seed_in The input block to be encrypted.
     * @param seed_out The output block after encryption.
     */
    void DoubleExpand(block seed_in, std::array<block, 2> &seed_out);

    /**
     * @brief Encrypts the input blocks using the PRG with separate keys.
     * @param seed_in The input blocks to be encrypted.
     * @param seed_out The output blocks after encryption with left and right keys.
     */
    void DoubleExpand(std::array<block, 8> &seed_in, std::array<std::array<block, 8>, 2> &seed_out);

private:
    emp::AES_KEY aes_key[2]; /**< AES key for the PRG. */
};

class PseudoRandomGeneratorSingleton {
public:
    static PseudoRandomGenerator &GetInstance();

    PseudoRandomGeneratorSingleton(const PseudoRandomGeneratorSingleton &)            = delete;
    PseudoRandomGeneratorSingleton &operator=(const PseudoRandomGeneratorSingleton &) = delete;

private:
    PseudoRandomGeneratorSingleton() = default;
};

}    // namespace prg
}    // namespace fss
}    // namespace fsswm

#endif    // PRG_PRG_H_
