#include "prg.h"

#include "FssWM/utils/block.h"

namespace {

const fsswm::block kPrgKeySeedLeft   = fsswm::MakeBlock(0x00, 0x00);
const fsswm::block kPrgKeySeedRight  = fsswm::MakeBlock(0x00, 0x01);
const fsswm::block kPrgKeyValueRight = fsswm::MakeBlock(0x01, 0x00);
const fsswm::block kPrgKeyValueLeft  = fsswm::MakeBlock(0x01, 0x01);

}    // namespace

namespace fsswm {
namespace fss {
namespace prg {

PseudoRandomGenerator::PseudoRandomGenerator(block init_seed0, block init_seed1,
                                             block init_seed2, block init_seed3) {
    aes_seed_[0].setKey(init_seed0);
    aes_seed_[1].setKey(init_seed1);
    aes_value_[0].setKey(init_seed2);
    aes_value_[1].setKey(init_seed3);
}

void PseudoRandomGenerator::Expand(block seed_in, block &seed_out, bool key_lr) {
    block tmp = seed_in;
    aes_seed_[key_lr].ecbEncBlock(tmp, tmp);
    seed_out = seed_in ^ tmp;
}

void PseudoRandomGenerator::ExpandValue(block seed_in, block &seed_out, bool key_lr) {
    block tmp = seed_in;
    aes_value_[key_lr].ecbEncBlock(tmp, tmp);
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

    aes_seed_[key_lr].ecbEncBlocks<8>(tmp.data(), tmp.data());

    seed_out[0] = seed_in[0] ^ tmp[0];
    seed_out[1] = seed_in[1] ^ tmp[1];
    seed_out[2] = seed_in[2] ^ tmp[2];
    seed_out[3] = seed_in[3] ^ tmp[3];
    seed_out[4] = seed_in[4] ^ tmp[4];
    seed_out[5] = seed_in[5] ^ tmp[5];
    seed_out[6] = seed_in[6] ^ tmp[6];
    seed_out[7] = seed_in[7] ^ tmp[7];
}

void PseudoRandomGenerator::DoubleExpand(block seed_in, std::array<block, 2> &seed_out) {
    block tmp[2];
    tmp[0] = seed_out[0] = seed_in;
    tmp[1] = seed_out[1] = seed_in;

    aes_seed_[0].ecbEncBlock(tmp[0], tmp[0]);
    aes_seed_[1].ecbEncBlock(tmp[1], tmp[1]);

    seed_out[0] = seed_out[0] ^ tmp[0];
    seed_out[1] = seed_out[1] ^ tmp[1];
}

void PseudoRandomGenerator::DoubleExpandValue(block seed_in, std::array<block, 2> &seed_out) {
    block tmp[2];
    tmp[0] = seed_out[0] = seed_in;
    tmp[1] = seed_out[1] = seed_in;

    aes_value_[0].ecbEncBlock(tmp[0], tmp[0]);
    aes_value_[1].ecbEncBlock(tmp[1], tmp[1]);

    seed_out[0] = seed_out[0] ^ tmp[0];
    seed_out[1] = seed_out[1] ^ tmp[1];
}

PseudoRandomGenerator &PseudoRandomGenerator::GetInstance() {
    static PseudoRandomGenerator instance(
        kPrgKeySeedLeft, kPrgKeySeedRight, kPrgKeyValueLeft, kPrgKeyValueRight);
    return instance;
}

}    // namespace prg
}    // namespace fss
}    // namespace fsswm
