#ifndef FSS_PRG_H_
#define FSS_PRG_H_

#include <cryptoTools/Crypto/AES.h>

#include "FssWM/utils/block.h"

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
    void Expand(std::array<block, 8> &seed_in, std::array<block, 8> &seed_out, bool key_lr);

    /**
     * @brief Encrypts the input block using the PRG.
     * @param seed_in The input block to be encrypted.
     * @param seed_out The output block after encryption.
     */
    void DoubleExpand(block seed_in, std::array<block, 2> &seed_out);

private:
    std::array<osuCrypto::AES, 2> aes_; /**< AES instances for the PRG from osuCrypto. */
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
