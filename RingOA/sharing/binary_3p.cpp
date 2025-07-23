#include "binary_3p.h"

#include <cryptoTools/Crypto/PRNG.h>

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace sharing {

BinaryReplicatedSharing3P::BinaryReplicatedSharing3P(const uint64_t bitsize)
    : bitsize_(bitsize) {
}

void BinaryReplicatedSharing3P::OfflineSetUp(const std::string &file_path) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Offline setup for BinaryReplicatedSharing3P.");
#endif
    RandOffline(file_path);
}

void BinaryReplicatedSharing3P::OnlineSetUp(const uint64_t party_id, const std::string &file_path) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Party " + ToString(party_id) + ": Online setup for BinaryReplicatedSharing3P.");
#endif
    RandOnline(party_id, file_path);
}

std::array<RepShare64, kThreeParties> BinaryReplicatedSharing3P::ShareLocal(const uint64_t &x) const {
    uint64_t x0 = GlobalRng::Rand<uint64_t>();
    uint64_t x1 = GlobalRng::Rand<uint64_t>();
    uint64_t x2 = x ^ x0 ^ x1;

    std::array<RepShare64, kThreeParties> all_shares;
    all_shares[0].data = {x0, x2};
    all_shares[1].data = {x1, x0};
    all_shares[2].data = {x2, x1};

    return all_shares;
}

std::array<RepShareVec64, kThreeParties> BinaryReplicatedSharing3P::ShareLocal(const std::vector<uint64_t> &x_vec) const {
    const size_t n = x_vec.size();

    std::vector<uint64_t> p0_0(n), p0_1(n);
    std::vector<uint64_t> p1_0(n), p1_1(n);
    std::vector<uint64_t> p2_0(n), p2_1(n);

    for (size_t i = 0; i < n; ++i) {
        uint64_t x  = x_vec[i];
        uint64_t r0 = GlobalRng::Rand<uint64_t>();
        uint64_t r1 = GlobalRng::Rand<uint64_t>();
        uint64_t r2 = x ^ r0 ^ r1;

        // P0: (r0, r2)
        p0_0[i] = r0;
        p0_1[i] = r2;
        // P1: (r1, r0)
        p1_0[i] = r1;
        p1_1[i] = r0;
        // P2: (r2, r1)
        p2_0[i] = r2;
        p2_1[i] = r1;
    }

    return {
        RepShareVec64(std::move(p0_0), std::move(p0_1)),
        RepShareVec64(std::move(p1_0), std::move(p1_1)),
        RepShareVec64(std::move(p2_0), std::move(p2_1))};
}

std::array<RepShareMat64, kThreeParties> BinaryReplicatedSharing3P::ShareLocal(const std::vector<uint64_t> &x_flat, size_t rows, size_t cols) const {
    const size_t          n = rows * cols;
    std::vector<uint64_t> p0_0(n), p0_1(n);
    std::vector<uint64_t> p1_0(n), p1_1(n);
    std::vector<uint64_t> p2_0(n), p2_1(n);

    for (size_t i = 0; i < n; ++i) {
        uint64_t x  = x_flat[i];
        uint64_t r0 = GlobalRng::Rand<uint64_t>();
        uint64_t r1 = GlobalRng::Rand<uint64_t>();
        uint64_t r2 = x ^ r0 ^ r1;

        // P0: (r0, r2)
        p0_0[i] = r0;
        p0_1[i] = r2;
        // P1: (r1, r0)
        p1_0[i] = r1;
        p1_1[i] = r0;
        // P2: (r2, r1)
        p2_0[i] = r2;
        p2_1[i] = r1;
    }

    return {
        RepShareMat64(rows, cols, std::move(p0_0), std::move(p0_1)),
        RepShareMat64(rows, cols, std::move(p1_0), std::move(p1_1)),
        RepShareMat64(rows, cols, std::move(p2_0), std::move(p2_1))};
}

std::array<RepShareBlock, kThreeParties> BinaryReplicatedSharing3P::ShareLocal(const block &x) const {
    block x0 = GlobalRng::Rand<block>();
    block x1 = GlobalRng::Rand<block>();
    block x2 = x ^ x0 ^ x1;

    std::array<RepShareBlock, kThreeParties> all_shares;
    all_shares[0].data = {x0, x2};
    all_shares[1].data = {x1, x0};
    all_shares[2].data = {x2, x1};

    return all_shares;
}

std::array<RepShareVecBlock, kThreeParties> BinaryReplicatedSharing3P::ShareLocal(const std::vector<block> &x_vec) const {
    const size_t n = x_vec.size();

    std::vector<block> p0_0(n), p0_1(n);
    std::vector<block> p1_0(n), p1_1(n);
    std::vector<block> p2_0(n), p2_1(n);

    for (size_t i = 0; i < n; ++i) {
        block x  = x_vec[i];
        block r0 = GlobalRng::Rand<block>();
        block r1 = GlobalRng::Rand<block>();
        block r2 = x ^ r0 ^ r1;

        // P0: (r0, r2)
        p0_0[i] = r0;
        p0_1[i] = r2;
        // P1: (r1, r0)
        p1_0[i] = r1;
        p1_1[i] = r0;
        // P2: (r2, r1)
        p2_0[i] = r2;
        p2_1[i] = r1;
    }

    return {
        RepShareVecBlock(std::move(p0_0), std::move(p0_1)),
        RepShareVecBlock(std::move(p1_0), std::move(p1_1)),
        RepShareVecBlock(std::move(p2_0), std::move(p2_1))};
}

std::array<RepShareMatBlock, kThreeParties> BinaryReplicatedSharing3P::ShareLocal(const std::vector<block> &x_flat, size_t rows, size_t cols) const {
    const size_t       n = rows * cols;
    std::vector<block> p0_0(n), p0_1(n);
    std::vector<block> p1_0(n), p1_1(n);
    std::vector<block> p2_0(n), p2_1(n);

    for (size_t i = 0; i < n; ++i) {
        block x  = x_flat[i];
        block r0 = GlobalRng::Rand<block>();
        block r1 = GlobalRng::Rand<block>();
        block r2 = x ^ r0 ^ r1;

        // P0: (r0, r2)
        p0_0[i] = r0;
        p0_1[i] = r2;
        // P1: (r1, r0)
        p1_0[i] = r1;
        p1_1[i] = r0;
        // P2: (r2, r1)
        p2_0[i] = r2;
        p2_1[i] = r1;
    }

    return {
        RepShareMatBlock(rows, cols, std::move(p0_0), std::move(p0_1)),
        RepShareMatBlock(rows, cols, std::move(p1_0), std::move(p1_1)),
        RepShareMatBlock(rows, cols, std::move(p2_0), std::move(p2_1))};
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShare64 &x_sh, uint64_t &open_x) const {
    // Send the first share to the previous party
    chls.prev.send(x_sh[0]);

    // Receive the share from the next party
    uint64_t x_next;
    chls.next.recv(x_next);

    // Sum the shares and compute the open value
    open_x = x_sh[0] ^ x_sh[1] ^ x_next;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Sent first share to the previous party: " + ToString(x_sh[0]));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Received share from the next party: " + ToString(x_next));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] (x_0, x_1, x_2): (" + ToString(x_sh[0]) + ", " + ToString(x_sh[1]) + ", " + ToString(x_next) + ")");
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareVec64 &x_vec_sh, std::vector<uint64_t> &open_x_vec) const {
    // Send the first share to the previous party
    chls.prev.send(x_vec_sh[0]);

    // Receive the share from the next party
    std::vector<uint64_t> x_vec_next;
    chls.next.recv(x_vec_next);

    // Sum the shares and compute the open values
    if (open_x_vec.size() != x_vec_sh.num_shares) {
        open_x_vec.resize(x_vec_sh.num_shares);
    }
    for (uint64_t i = 0; i < open_x_vec.size(); ++i) {
        open_x_vec[i] = x_vec_sh[0][i] ^ x_vec_sh[1][i] ^ x_vec_next[i];
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Sent first share to the previous party: " + ToString(x_vec_sh[0]));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Received share from the next party: " + ToString(x_vec_next));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] x_0: " + ToString(x_vec_sh[0]) + ", x_1: " + ToString(x_vec_sh[1]) + ", x_2: " + ToString(x_vec_next));
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareMat64 &x_mat_sh, std::vector<uint64_t> &open_x_flat) const {
    const size_t rows = x_mat_sh.rows;
    const size_t cols = x_mat_sh.cols;
    const size_t n    = rows * cols;
    // Send the first share to the previous party
    chls.prev.send(x_mat_sh[0]);

    // Receive the shares from the next party
    std::vector<uint64_t> x_mat_next(n);
    chls.next.recv(x_mat_next);

    // Sum the shares and compute the open values
    if (open_x_flat.size() != n) {
        open_x_flat.resize(n);
    }
    for (size_t i = 0; i < n; ++i) {
        open_x_flat[i] = x_mat_sh[0][i] ^ x_mat_sh[1][i] ^ x_mat_next[i];
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Sent first share to the previous party: " + ToStringMatrix(x_mat_sh[0], rows, cols));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Received share from the next party: " + ToStringMatrix(x_mat_next, rows, cols));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] x_0: " + ToStringMatrix(x_mat_sh[0], rows, cols) +
                              ", x_1: " + ToStringMatrix(x_mat_sh[1], rows, cols) + ", x_2: " + ToStringMatrix(x_mat_next, rows, cols));
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareBlock &x_sh, block &open_x) const {
    // Send the first share to the previous party
    chls.prev.send(x_sh[0]);

    // Receive the share from the next party
    block x_next;
    chls.next.recv(x_next);

    // Sum the shares and compute the open value
    open_x = x_sh[0] ^ x_sh[1] ^ x_next;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Sent first share to the previous party: " + Format(x_sh[0]));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Received share from the next party: " + Format(x_next));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] (x_0, x_1, x_2): (" + Format(x_sh[0]) + ", " + Format(x_sh[1]) + ", " + Format(x_next) + ")");
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareVecBlock &x_vec_sh, std::vector<block> &open_x_vec) const {
    // Send the first share to the previous party
    chls.prev.send(x_vec_sh[0]);

    // Receive the share from the next party
    std::vector<block> x_vec_next;
    chls.next.recv(x_vec_next);

    // Sum the shares and compute the open values
    if (open_x_vec.size() != x_vec_sh.num_shares) {
        open_x_vec.resize(x_vec_sh.num_shares);
    }
    for (uint64_t i = 0; i < open_x_vec.size(); ++i) {
        open_x_vec[i] = x_vec_sh[0][i] ^ x_vec_sh[1][i] ^ x_vec_next[i];
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Sent first share to the previous party: " + Format(x_vec_sh[0]));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Received share from the next party: " + Format(x_vec_next));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] x_0: " + Format(x_vec_sh[0]) + ", x_1: " + Format(x_vec_sh[1]) + ", x_2: " + Format(x_vec_next));
#endif
}

void BinaryReplicatedSharing3P::Open(Channels &chls, const RepShareMatBlock &x_mat_sh, std::vector<block> &open_x_flat) const {
    const size_t rows = x_mat_sh.rows;
    const size_t cols = x_mat_sh.cols;
    const size_t n    = rows * cols;
    // Send the first share to the previous party
    chls.prev.send(x_mat_sh[0]);

    // Receive the shares from the next party
    std::vector<block> x_mat_next(n);
    chls.next.recv(x_mat_next);

    // Sum the shares and compute the open values
    if (open_x_flat.size() != n) {
        open_x_flat.resize(n);
    }
    for (size_t i = 0; i < n; ++i) {
        open_x_flat[i] = x_mat_sh[0][i] ^ x_mat_sh[1][i] ^ x_mat_next[i];
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Sent first share to the previous party: " + FormatMatrix(x_mat_sh[0], rows, cols));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] Received share from the next party: " + FormatMatrix(x_mat_next, rows, cols));
    Logger::DebugLog(LOC, "[P" + ToString(chls.party_id) + "] x_0: " + FormatMatrix(x_mat_sh[0], rows, cols) +
                              ", x_1: " + FormatMatrix(x_mat_sh[1], rows, cols) + ", x_2: " + FormatMatrix(x_mat_next, rows, cols));
#endif
}

void BinaryReplicatedSharing3P::Rand(RepShare64 &x) {
    constexpr size_t ELEM_SIZE = sizeof(uint64_t);
    size_t           max_elems = prf_buff_[0].size() * sizeof(block) / ELEM_SIZE;
    size_t           elem_idx  = prf_idx_ / ELEM_SIZE;
    if (elem_idx >= max_elems) {
        RefillBuffer();
        elem_idx = 0;
        prf_idx_ = 0;
    }

    uint64_t tmp;
    std::memcpy(&tmp, reinterpret_cast<const uint8_t *>(prf_buff_[0].data()) + prf_idx_, ELEM_SIZE);
    x.data[0] = tmp;
    std::memcpy(&tmp, reinterpret_cast<const uint8_t *>(prf_buff_[1].data()) + prf_idx_, ELEM_SIZE);
    x.data[1] = tmp;
    prf_idx_ += ELEM_SIZE;
}

void BinaryReplicatedSharing3P::Rand(RepShareBlock &x) {
    constexpr size_t ELEM_SIZE = sizeof(block);
    size_t           max_elems = prf_buff_[0].size();
    size_t           elem_idx  = prf_idx_ / ELEM_SIZE;
    if (elem_idx >= max_elems) {
        RefillBuffer();
        elem_idx = 0;
        prf_idx_ = 0;
    }

    std::memcpy(&x.data[0], reinterpret_cast<const uint8_t *>(prf_buff_[0].data()) + prf_idx_, ELEM_SIZE);
    std::memcpy(&x.data[1], reinterpret_cast<const uint8_t *>(prf_buff_[1].data()) + prf_idx_, ELEM_SIZE);
    prf_idx_ += ELEM_SIZE;
}

uint64_t BinaryReplicatedSharing3P::GenerateRandomValue() const {
    return Mod(GlobalRng::Rand<uint64_t>(), bitsize_);
}

void BinaryReplicatedSharing3P::EvaluateXor(const RepShare64 &x_sh, const RepShare64 &y_sh, RepShare64 &z_sh) const {
    z_sh.data[0] = x_sh.data[0] ^ y_sh.data[0];
    z_sh.data[1] = x_sh.data[1] ^ y_sh.data[1];
}

void BinaryReplicatedSharing3P::EvaluateXor(const RepShareVec64 &x_vec_sh, const RepShareVec64 &y_vec_sh, RepShareVec64 &z_vec_sh) const {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateXor.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    for (uint64_t i = 0; i < x_vec_sh.num_shares; ++i) {
        z_vec_sh.data[0][i] = x_vec_sh.data[0][i] ^ y_vec_sh.data[0][i];
        z_vec_sh.data[1][i] = x_vec_sh.data[1][i] ^ y_vec_sh.data[1][i];
    }
}

void BinaryReplicatedSharing3P::EvaluateAnd(Channels &chls, const RepShare64 &x_sh, const RepShare64 &y_sh, RepShare64 &z_sh) {
    // (t_0, t_1, t_2) forms a (3, 3)-sharing of t = x & y
    uint64_t   t_sh = (x_sh.data[0] & y_sh.data[0]) ^ (x_sh.data[1] & y_sh.data[0]) ^ (x_sh.data[0] & y_sh.data[1]);
    RepShare64 r_sh;
    Rand(r_sh);
    z_sh.data[0] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    chls.next.send(z_sh.data[0]);
    chls.prev.recv(z_sh.data[1]);
}

void BinaryReplicatedSharing3P::EvaluateAnd(Channels &chls, const RepShareVec64 &x_vec_sh, const RepShareVec64 &y_vec_sh, RepShareVec64 &z_vec_sh) {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateAnd.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    for (uint64_t i = 0; i < x_vec_sh.num_shares; ++i) {
        // (t_0, t_1, t_2) forms a (3, 3)-sharing of t = x & y
        uint64_t   t_sh = (x_vec_sh.data[0][i] & y_vec_sh.data[0][i]) ^ (x_vec_sh.data[1][i] & y_vec_sh.data[0][i]) ^ (x_vec_sh.data[0][i] & y_vec_sh.data[1][i]);
        RepShare64 r_sh;
        Rand(r_sh);
        z_vec_sh.data[0][i] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    }

    chls.next.send(z_vec_sh.data[0]);
    chls.prev.recv(z_vec_sh.data[1]);
}

void BinaryReplicatedSharing3P::EvaluateSelect(Channels &chls, const RepShare64 &x_sh, const RepShare64 &y_sh, const RepShare64 &c_sh, RepShare64 &z_sh) {
    RepShare64 xy_sh;
    EvaluateXor(x_sh, y_sh, xy_sh);
    RepShare64 c_and_xy_sh;
    EvaluateAnd(chls, c_sh, xy_sh, c_and_xy_sh);
    EvaluateXor(x_sh, c_and_xy_sh, z_sh);
}

void BinaryReplicatedSharing3P::EvaluateSelect(Channels &chls, const RepShareVec64 &x_vec_sh, const RepShareVec64 &y_vec_sh, const RepShare64 &c_sh, RepShareVec64 &z_vec_sh) {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateSelect.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    const size_t  n = x_vec_sh.num_shares;
    RepShareVec64 xy_sh(n);
    EvaluateXor(x_vec_sh, y_vec_sh, xy_sh);
    RepShareVec64 c_and_xy_sh(n);

    for (uint64_t i = 0; i < n; ++i) {
        uint64_t   t_sh = (xy_sh.data[0][i] & c_sh.data[0]) ^ (xy_sh.data[1][i] & c_sh.data[0]) ^ (xy_sh.data[0][i] & c_sh.data[1]);
        RepShare64 r_sh;
        Rand(r_sh);
        c_and_xy_sh.data[0][i] = t_sh ^ r_sh.data[0] ^ r_sh.data[1];
    }

    chls.next.send(c_and_xy_sh.data[0]);
    chls.prev.recv(c_and_xy_sh.data[1]);

    EvaluateXor(x_vec_sh, c_and_xy_sh, z_vec_sh);
}

void BinaryReplicatedSharing3P::RandOffline(const std::string &file_path) const {
    block                            key_0 = GlobalRng::Rand<block>();
    block                            key_1 = GlobalRng::Rand<block>();
    block                            key_2 = GlobalRng::Rand<block>();
    std::array<block, kThreeParties> keys  = {key_0, key_1, key_2};

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Offline Rand for BinaryReplicatedSharing3P.");
    for (uint64_t i = 0; i < kThreeParties; ++i) {
        Logger::DebugLog(LOC, "[P" + ToString(i) + "] Prf keys (i): " + Format(keys[i]) + ", (i-1): " + Format(keys[(i + 2) % kThreeParties]));
    }
#endif

    try {
        // Save the keys to the file
        // next: i, prev: i-1
        FileIo io(".key");
        io.WriteBinary(file_path + "_next_0", keys[0]);
        io.WriteBinary(file_path + "_prev_0", keys[2]);
        io.WriteBinary(file_path + "_next_1", keys[1]);
        io.WriteBinary(file_path + "_prev_1", keys[0]);
        io.WriteBinary(file_path + "_next_2", keys[2]);
        io.WriteBinary(file_path + "_prev_2", keys[1]);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "PRF keys written to file: " + file_path + "_next_0.key.bin");
        Logger::DebugLog(LOC, "PRF keys written to file: " + file_path + "_prev_0.key.bin");
        Logger::DebugLog(LOC, "PRF keys written to file: " + file_path + "_next_1.key.bin");
        Logger::DebugLog(LOC, "PRF keys written to file: " + file_path + "_prev_1.key.bin");
        Logger::DebugLog(LOC, "PRF keys written to file: " + file_path + "_next_2.key.bin");
        Logger::DebugLog(LOC, "PRF keys written to file: " + file_path + "_prev_2.key.bin");
#endif

    } catch (const std::exception &e) {
        Logger::FatalLog(LOC, "Failed to write PRF keys to file: " + std::string(e.what()));
        exit(EXIT_FAILURE);
    }
}

void BinaryReplicatedSharing3P::RandOnline(const uint64_t party_id, const std::string &file_path, uint64_t buffer_size) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Rand setup for BinaryReplicatedSharing3P.");
#endif

    block key_next, key_prev;
    try {
        // Load the keys from the file
        FileIo io(".key");
        io.ReadBinary(file_path + "_next_" + ToString(party_id), key_next);
        io.ReadBinary(file_path + "_prev_" + ToString(party_id), key_prev);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "PRF keys read from file: " + file_path + "_next_" + ToString(party_id) + ".key.bin");
        Logger::DebugLog(LOC, "PRF keys read from file: " + file_path + "_prev_" + ToString(party_id) + ".key.bin");
#endif
    } catch (const std::exception &e) {
        Logger::FatalLog(LOC, "Failed to read PRF keys from file: " + std::string(e.what()));
        exit(EXIT_FAILURE);
    }
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
