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

std::array<RepShare, kNumParties> BinaryReplicatedSharing3P::ShareLocal(const uint32_t &x) const {
    uint32_t x0 = Mod(SecureRng::Rand32(), bitsize_);
    uint32_t x1 = Mod(SecureRng::Rand32(), bitsize_);
    uint32_t x2 = x ^ x0 ^ x1;

    std::array<RepShare, kNumParties> all_shares;
    all_shares[0].data = {x0, x2};
    all_shares[1].data = {x1, x0};
    all_shares[2].data = {x2, x1};

    return all_shares;
}

std::array<RepShareVec, kNumParties> BinaryReplicatedSharing3P::ShareLocal(const UIntVec &x_vec) const {
    size_t                               num_shares = x_vec.size();
    std::array<RepShareVec, kNumParties> all_shares;

    for (size_t p = 0; p < kNumParties; ++p) {
        all_shares[p].num_shares = num_shares;
        all_shares[p][0].resize(num_shares);
        all_shares[p][1].resize(num_shares);
    }

    for (size_t i = 0; i < num_shares; ++i) {
        all_shares[0][0][i] = Mod(SecureRng::Rand32(), bitsize_);
        all_shares[1][0][i] = Mod(SecureRng::Rand32(), bitsize_);
        all_shares[2][0][i] = x_vec[i] ^ all_shares[0][0][i] ^ all_shares[1][0][i];

        all_shares[0][1][i] = all_shares[2][0][i];
        all_shares[1][1][i] = all_shares[0][0][i];
        all_shares[2][1][i] = all_shares[1][0][i];
    }
    return all_shares;
}

std::array<RepShareMat, kNumParties> BinaryReplicatedSharing3P::ShareLocal(const UIntMat &x_mat) const {
    size_t                               rows = x_mat.size();
    size_t                               cols = x_mat.empty() ? 0 : x_mat[0].size();
    std::array<RepShareMat, kNumParties> all_shares;

    for (size_t p = 0; p < kNumParties; ++p) {
        all_shares[p].rows = rows;
        all_shares[p].cols = cols;
        all_shares[p][0].resize(rows, std::vector<uint32_t>(cols));
        all_shares[p][1].resize(rows, std::vector<uint32_t>(cols));
    }

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            all_shares[0][0][i][j] = Mod(SecureRng::Rand32(), bitsize_);
            all_shares[1][0][i][j] = Mod(SecureRng::Rand32(), bitsize_);
            all_shares[2][0][i][j] = x_mat[i][j] ^ all_shares[0][0][i][j] ^ all_shares[1][0][i][j];

            all_shares[0][1][i][j] = all_shares[2][0][i][j];
            all_shares[1][1][i][j] = all_shares[0][0][i][j];
            all_shares[2][1][i][j] = all_shares[1][0][i][j];
        }
    }
    return all_shares;
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShare &x_sh, uint32_t &open_x) const {
    // Send the first share to the previous party
    chls.prev.send(x_sh[0]);

    // Receive the share from the next party
    uint32_t x_next;
    chls.next.recv(x_next);

    // Sum the shares and compute the open value
    open_x = x_sh[0] ^ x_sh[1] ^ x_next;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + std::to_string(x_sh[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + std::to_string(x_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] (x_0, x_1, x_2): (" + std::to_string(x_sh[0]) + ", " + std::to_string(x_sh[1]) + ", " + std::to_string(x_next) + ")");
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareVec &x_vec_sh, UIntVec &open_x_vec) const {
    // Send the first share to the previous party
    chls.prev.send(x_vec_sh[0]);

    // Receive the share from the next party
    std::vector<uint32_t> x_vec_next;
    chls.next.recv(x_vec_next);

    // Sum the shares and compute the open values
    if (open_x_vec.size() != x_vec_sh.num_shares) {
        open_x_vec.resize(x_vec_sh.num_shares);
    }
    for (uint32_t i = 0; i < open_x_vec.size(); ++i) {
        open_x_vec[i] = x_vec_sh[0][i] ^ x_vec_sh[1][i] ^ x_vec_next[i];
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + ToString(x_vec_sh[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + ToString(x_vec_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] x_0: " + ToString(x_vec_sh[0]) + ", x_1: " + ToString(x_vec_sh[1]) + ", x_2: " + ToString(x_vec_next));
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareMat &x_mat_sh, UIntMat &open_x_mat) const {
    // Send the first share to the previous party
    for (const auto &row : x_mat_sh[0]) {
        chls.prev.send(row);
    }

    // Receive the shares from the next party
    UIntMat x_mat_next;
    for (size_t i = 0; i < x_mat_sh.rows; ++i) {
        UIntVec row(x_mat_sh.cols);
        chls.next.recv(row);
        x_mat_next.push_back(row);
    }

    // Sum the shares and compute the open values
    if (open_x_mat.size() != x_mat_sh.rows) {
        open_x_mat.resize(x_mat_sh.rows, UIntVec(x_mat_sh.cols));
    }
    for (size_t i = 0; i < x_mat_sh.rows; ++i) {
        for (size_t j = 0; j < x_mat_sh.cols; ++j) {
            open_x_mat[i][j] = x_mat_sh[0][i][j] ^ x_mat_sh[1][i][j] ^ x_mat_next[i][j];
        }
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + ToStringMat(x_mat_sh[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + ToStringMat(x_mat_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] x_0: " + ToStringMat(x_mat_sh[0]) + ", x_1: " + ToStringMat(x_mat_sh[1]) + ", x_2: " + ToStringMat(x_mat_next));
#endif
}

void BinaryReplicatedSharing3P::Rand(RepShare &x) {
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

void BinaryReplicatedSharing3P::EvaluateXor(const RepShare &x_sh, const RepShare &y_sh, RepShare &z_sh) const {
    z_sh.data[0] = x_sh.data[0] ^ y_sh.data[0];
    z_sh.data[1] = x_sh.data[1] ^ y_sh.data[1];
}

void BinaryReplicatedSharing3P::EvaluateXor(const RepShareVec &x_vec_sh, const RepShareVec &y_vec_sh, RepShareVec &z_vec_sh) const {
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

void BinaryReplicatedSharing3P::EvaluateAnd(Channels &chls, const RepShare &x_sh, const RepShare &y_sh, RepShare &z_sh) {
    // (t_0, t_1, t_2) forms a (3, 3)-sharing of t = x & y
    uint32_t t_sh = (x_sh.data[0] & y_sh.data[0]) ^ (x_sh.data[1] & y_sh.data[0]) ^ (x_sh.data[0] & y_sh.data[1]);
    RepShare r_sh;
    Rand(r_sh);
    z_sh.data[0] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    chls.next.send(z_sh.data[0]);
    chls.prev.recv(z_sh.data[1]);
}

void BinaryReplicatedSharing3P::EvaluateAnd(Channels &chls, const RepShareVec &x_vec_sh, const RepShareVec &y_vec_sh, RepShareVec &z_vec_sh) {
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
        uint32_t t_sh = (x_vec_sh.data[0][i] & y_vec_sh.data[0][i]) ^ (x_vec_sh.data[1][i] & y_vec_sh.data[0][i]) ^ (x_vec_sh.data[0][i] & y_vec_sh.data[1][i]);
        RepShare r_sh;
        Rand(r_sh);
        z_vec_sh.data[0][i] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    }

    chls.next.send(z_vec_sh.data[0]);
    chls.prev.recv(z_vec_sh.data[1]);
}

void BinaryReplicatedSharing3P::EvaluateSelect(Channels &chls, const RepShare &x_sh, const RepShare &y_sh, const RepShare &c_sh, RepShare &z_sh) {
    RepShare xy_sh;
    EvaluateXor(x_sh, y_sh, xy_sh);
    RepShare c_and_xy_sh;
    EvaluateAnd(chls, c_sh, xy_sh, c_and_xy_sh);
    EvaluateXor(x_sh, c_and_xy_sh, z_sh);
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
