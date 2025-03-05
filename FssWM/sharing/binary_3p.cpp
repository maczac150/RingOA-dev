#include "binary_3p.h"

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/utils.h"
#include "cryptoTools/Crypto/PRNG.h"

namespace fsswm {
namespace sharing {

BinaryReplicatedSharing3P::BinaryReplicatedSharing3P(const uint32_t bitsize)
    : bitsize_(bitsize) {
}

void BinaryReplicatedSharing3P::OfflineSetUp(const std::string &file_path) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Offline setup for BinaryReplicatedSharing3P.");
#endif
    RandOffline(file_path);
}

void BinaryReplicatedSharing3P::OnlineSetUp(const uint32_t party_id, const std::string &file_path) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Party " + std::to_string(party_id) + ": Online setup for BinaryReplicatedSharing3P.");
#endif
    RandOnline(party_id, file_path);
}

std::array<SharePair, kNumParties> BinaryReplicatedSharing3P::ShareLocal(const uint32_t &x) const {
    std::array<uint32_t, kNumParties> x_sh;
    x_sh[0] = Mod(SecureRng::Rand32(), bitsize_);
    x_sh[1] = Mod(SecureRng::Rand32(), bitsize_);
    x_sh[2] = x ^ x_sh[0] ^ x_sh[1];

    std::array<SharePair, kNumParties> all_shares;
    all_shares[0].data = {x_sh[0], x_sh[2]};
    all_shares[1].data = {x_sh[1], x_sh[0]};
    all_shares[2].data = {x_sh[2], x_sh[1]};

    return all_shares;
}

std::array<SharesPair, kNumParties> BinaryReplicatedSharing3P::ShareLocal(const std::vector<uint32_t> &x_vec) const {
    size_t num_shares = x_vec.size();

    std::array<std::vector<uint32_t>, kNumParties> x_sh_vec;
    x_sh_vec[0].resize(num_shares);
    x_sh_vec[1].resize(num_shares);
    x_sh_vec[2].resize(num_shares);

    for (uint32_t i = 0; i < x_vec.size(); i++) {
        x_sh_vec[0][i] = Mod(SecureRng::Rand32(), bitsize_);
        x_sh_vec[1][i] = Mod(SecureRng::Rand32(), bitsize_);
        x_sh_vec[2][i] = x_vec[i] ^ x_sh_vec[0][i] ^ x_sh_vec[1][i];
    }

    std::array<SharesPair, kNumParties> all_shares;
    all_shares[0].num_shares = num_shares;
    all_shares[1].num_shares = num_shares;
    all_shares[2].num_shares = num_shares;
    all_shares[0].data[0]    = x_sh_vec[0];
    all_shares[0].data[1]    = x_sh_vec[2];
    all_shares[1].data[0]    = x_sh_vec[1];
    all_shares[1].data[1]    = x_sh_vec[0];
    all_shares[2].data[0]    = x_sh_vec[2];
    all_shares[2].data[1]    = x_sh_vec[1];

    return all_shares;
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const SharePair &x_sh, uint32_t &open_x) const {
    // Send the first share to the previous party
    chls.prev.send(x_sh.data[0]);

    // Receive the share from the next party
    uint32_t x_next;
    chls.next.recv(x_next);

    // Sum the shares and compute the open value
    open_x = x_sh.data[0] ^ x_sh.data[1] ^ x_next;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + std::to_string(x_sh.data[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + std::to_string(x_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] (x_0, x_1, x_2): (" + std::to_string(x_sh.data[0]) + ", " + std::to_string(x_sh.data[1]) + ", " + std::to_string(x_next) + ")");
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const SharesPair &x_vec_sh, std::vector<uint32_t> &open_x_vec) const {
    // Send the first share to the previous party
    chls.prev.send(x_vec_sh.data[0]);

    // Receive the share from the next party
    std::vector<uint32_t> x_vec_next;
    chls.next.recv(x_vec_next);

    // Sum the shares and compute the open values
    if (open_x_vec.size() != x_vec_sh.num_shares) {
        open_x_vec.resize(x_vec_sh.num_shares);
    }
    for (uint32_t i = 0; i < open_x_vec.size(); ++i) {
        open_x_vec[i] = x_vec_sh.data[0][i] ^ x_vec_sh.data[1][i] ^ x_vec_next[i];
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + ToString(x_vec_sh.data[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + ToString(x_vec_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] x_0: " + ToString(x_vec_sh.data[0]) + ", x_1: " + ToString(x_vec_sh.data[1]) + ", x_2: " + ToString(x_vec_next));
#endif
}

void BinaryReplicatedSharing3P::Rand(SharePair &x) {
    if (prf_idx_ + sizeof(uint32_t) > prf_buff_[0].size() * sizeof(oc::block)) {
        RefillBuffer();
    }
    x.data[0] = Mod(*(uint32_t *)((uint8_t *)prf_buff_[0].data() + prf_idx_), bitsize_);
    x.data[1] = Mod(*(uint32_t *)((uint8_t *)prf_buff_[1].data() + prf_idx_), bitsize_);
    prf_idx_ += sizeof(uint32_t);
}

uint32_t BinaryReplicatedSharing3P::GenerateRandomValue() const {
    return Mod(SecureRng::Rand32(), bitsize_);
}

void BinaryReplicatedSharing3P::EvaluateXor(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const {
    z_sh.data[0] = x_sh.data[0] ^ y_sh.data[0];
    z_sh.data[1] = x_sh.data[1] ^ y_sh.data[1];
}

void BinaryReplicatedSharing3P::EvaluateXor(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateXor.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    for (uint32_t i = 0; i < x_vec_sh.num_shares; ++i) {
        z_vec_sh.data[0][i] = x_vec_sh.data[0][i] ^ y_vec_sh.data[0][i];
        z_vec_sh.data[1][i] = x_vec_sh.data[1][i] ^ y_vec_sh.data[1][i];
    }
}

void BinaryReplicatedSharing3P::EvaluateAnd(Channels &chls, const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) {
    // (t_0, t_1, t_2) forms a (3, 3)-sharing of t = x & y
    uint32_t  t_sh = (x_sh.data[0] & y_sh.data[0]) ^ (x_sh.data[1] & y_sh.data[0]) ^ (x_sh.data[0] & y_sh.data[1]);
    SharePair r_sh;
    Rand(r_sh);
    z_sh.data[0] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    chls.next.send(z_sh.data[0]);
    chls.prev.recv(z_sh.data[1]);
}

void BinaryReplicatedSharing3P::EvaluateAnd(Channels &chls, const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateAnd.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    for (uint32_t i = 0; i < x_vec_sh.num_shares; ++i) {
        // (t_0, t_1, t_2) forms a (3, 3)-sharing of t = x & y
        uint32_t  t_sh = (x_vec_sh.data[0][i] & y_vec_sh.data[0][i]) ^ (x_vec_sh.data[1][i] & y_vec_sh.data[0][i]) ^ (x_vec_sh.data[0][i] & y_vec_sh.data[1][i]);
        SharePair r_sh;
        Rand(r_sh);
        z_vec_sh.data[0][i] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    }

    chls.next.send(z_vec_sh.data[0]);
    chls.prev.recv(z_vec_sh.data[1]);
}

void BinaryReplicatedSharing3P::RandOffline(const std::string &file_path) const {
    Logger::DebugLog(LOC, "Offline Rand for BinaryReplicatedSharing3P.");
    std::array<uint64_t, 2>                          key_0 = {SecureRng::Rand64(), SecureRng::Rand64()};
    std::array<uint64_t, 2>                          key_1 = {SecureRng::Rand64(), SecureRng::Rand64()};
    std::array<uint64_t, 2>                          key_2 = {SecureRng::Rand64(), SecureRng::Rand64()};
    std::array<std::array<uint64_t, 2>, kNumParties> keys  = {key_0, key_1, key_2};

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint32_t i = 0; i < kNumParties; ++i) {
        Logger::DebugLog(LOC, "[P" + std::to_string(i) + "] Prf keys (i): " + ToString(keys[i], FormatType::kHex) + ", (i-1): " + ToString(keys[(i + 2) % kNumParties], FormatType::kHex));
    }
#endif

    // Save the keys to the file
    FileIo io(".key");
    // next: i, prev: i-1
    io.WriteToFileBinary(file_path + "_next_0", keys[0]);
    io.WriteToFileBinary(file_path + "_prev_0", keys[2]);
    io.WriteToFileBinary(file_path + "_next_1", keys[1]);
    io.WriteToFileBinary(file_path + "_prev_1", keys[0]);
    io.WriteToFileBinary(file_path + "_next_2", keys[2]);
    io.WriteToFileBinary(file_path + "_prev_2", keys[1]);
}

void BinaryReplicatedSharing3P::RandOnline(const uint32_t party_id, const std::string &file_path, uint32_t buffer_size) {
    Logger::DebugLog(LOC, "Rand setup for BinaryReplicatedSharing3P.");

    // Load the keys from the file
    FileIo                  io(".key");
    std::array<uint64_t, 2> key_next, key_prev;
    io.ReadFromFileBinary(file_path + "_next_" + std::to_string(party_id), key_next);
    io.ReadFromFileBinary(file_path + "_prev_" + std::to_string(party_id), key_prev);

    // Initialize PRF
    prf_buff_idx_ = 0;
    prf_buff_[0].resize(buffer_size);
    prf_buff_[1].resize(buffer_size);
    prf_[0].setKey(key_prev);
    prf_[1].setKey(key_next);

    // Refill the buffer
    RefillBuffer();
}

void BinaryReplicatedSharing3P::RefillBuffer() {
    prf_[0].ecbEncCounterMode(prf_buff_idx_, prf_buff_[0].size(), prf_buff_[0].data());
    prf_[1].ecbEncCounterMode(prf_buff_idx_, prf_buff_[1].size(), prf_buff_[1].data());
    prf_buff_idx_ += prf_buff_[0].size();
    prf_idx_ = 0;
}

}    // namespace sharing
}    // namespace fsswm
