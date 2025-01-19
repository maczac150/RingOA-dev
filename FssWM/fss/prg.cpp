#include "prg.h"

#include <emp-tool/utils/aes_opt.h>

#include "FssWM/utils/block.h"

namespace {

const fsswm::block kPrgKeySeedLeft  = fsswm::makeBlock(0xf2416bf54f02e446, 0xcc2ce93fdbcccc28);    // First half of the SHA256 hash for `dpf::kPrgKeySeedLeft`
const fsswm::block kPrgKeySeedRight = fsswm::makeBlock(0x65776b0991b8d225, 0xdac18583c2123349);    // First half of the SHA256 hash for `dpf::kPrgKeySeedRight`

}    // namespace

namespace fsswm {
namespace fss {
namespace prg {

PseudoRandomGenerator::PseudoRandomGenerator(block init_seed0, block init_seed1) {
    emp::AES_set_encrypt_key(reinterpret_cast<const block &>(init_seed0), aes_key);
    emp::AES_set_encrypt_key(reinterpret_cast<const block &>(init_seed1), &aes_key[1]);
}

void PseudoRandomGenerator::Expand(block seed_in, block &seed_out, bool key_lr) {
    block tmp = seed_in;
    emp::AES_ecb_encrypt_blks(&tmp, 1, &aes_key[key_lr]);
    seed_out = seed_in ^ tmp;
}

void PseudoRandomGenerator::Expand(std::array<block, 8> &seed_in, std::array<block, 8> &seed_out, bool key_lr) {
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

void PseudoRandomGenerator::Expand(std::array<block, 16> &seed_in, std::array<block, 16> &seed_out, bool key_lr) {
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

void PseudoRandomGenerator::DoubleExpand(block seed_in, std::array<block, 2> &seed_out) {
    block tmp[2];
    tmp[0] = seed_out[0] = seed_in;
    tmp[1] = seed_out[1] = seed_in;

    emp::ParaEnc<2, 1>(tmp, aes_key);

    seed_out[0] = seed_out[0] ^ tmp[0];
    seed_out[1] = seed_out[1] ^ tmp[1];
}

void PseudoRandomGenerator::DoubleExpand(std::array<block, 8> &seed_in, std::array<std::array<block, 8>, 2> &seed_out) {
    for (uint32_t i = 0; i < 8; ++i) {
        block tmp[2];
        tmp[0] = seed_out[0][i] = seed_in[i];
        tmp[1] = seed_out[1][i] = seed_in[i];

        emp::ParaEnc<2, 1>(tmp, aes_key);

        seed_out[0][i] = seed_out[0][i] ^ tmp[0];
        seed_out[1][i] = seed_out[1][i] ^ tmp[1];
    }
}

PseudoRandomGenerator &PseudoRandomGeneratorSingleton::GetInstance() {
    static PseudoRandomGenerator instance(kPrgKeySeedLeft, kPrgKeySeedRight);
    return instance;
}

}    // namespace prg
}    // namespace fss
}    // namespace fsswm
