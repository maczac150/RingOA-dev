#include "additive_3p.h"

#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Common/block.h"
#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace sharing {

using fsswm::utils::Logger;
using fsswm::utils::Mod;
using fsswm::utils::SecureRng;
using fsswm::utils::ToString;

ReplicatedSharing3P::ReplicatedSharing3P(const uint32_t bitsize)
    : bitsize_(bitsize) {
}

void ReplicatedSharing3P::OfflineSetUp(Channels &chls, const std::string &file_path) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Offline setup for ReplicatedSharing3P.");
#endif
    RandSetup(chls);
}

std::array<SharePair, kNumParties> ReplicatedSharing3P::ShareLocal(const uint32_t &x) const {
    std::array<uint32_t, kNumParties> x_sh;
    x_sh[0] = Mod(SecureRng::Rand32(), bitsize_);
    x_sh[1] = Mod(SecureRng::Rand32(), bitsize_);
    x_sh[2] = Mod(x - x_sh[0] - x_sh[1], bitsize_);

    std::array<SharePair, kNumParties> all_shares;
    all_shares[0].data = {x_sh[0], x_sh[2]};
    all_shares[1].data = {x_sh[1], x_sh[0]};
    all_shares[2].data = {x_sh[2], x_sh[1]};

    return all_shares;
}

std::array<SharesPair, kNumParties> ReplicatedSharing3P::ShareLocal(const std::vector<uint32_t> &x_vec) const {
    size_t num_shares = x_vec.size();

    std::array<std::vector<uint32_t>, kNumParties> x_sh_vec;
    x_sh_vec[0].resize(num_shares);
    x_sh_vec[1].resize(num_shares);
    x_sh_vec[2].resize(num_shares);

    for (uint32_t i = 0; i < x_vec.size(); i++) {
        x_sh_vec[0][i] = Mod(SecureRng::Rand32(), bitsize_);
        x_sh_vec[1][i] = Mod(SecureRng::Rand32(), bitsize_);
        x_sh_vec[2][i] = Mod(x_vec[i] - x_sh_vec[0][i] - x_sh_vec[1][i], bitsize_);
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

void ReplicatedSharing3P::Open(Channels &chls, const SharePair &x_sh, uint32_t &open_x) const {
    // Send the first share to the previous party
    chls.prev.send(x_sh.data[0]);

    // Receive the share from the next party
    uint32_t x_next;
    chls.next.recv(x_next);

    // Sum the shares and compute the open value
    open_x = Mod(x_sh.data[0] + x_sh.data[1] + x_next, bitsize_);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + std::to_string(x_sh.data[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + std::to_string(x_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] (x_0, x_1, x_2): (" + std::to_string(x_sh.data[0]) + ", " + std::to_string(x_sh.data[1]) + ", " + std::to_string(x_next) + ")");
#endif
}

void ReplicatedSharing3P::Open(Channels &chls, const SharesPair &x_vec_sh, std::vector<uint32_t> &open_x_vec) const {
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
        open_x_vec[i] = Mod(x_vec_sh.data[0][i] + x_vec_sh.data[1][i] + x_vec_next[i], bitsize_);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Sent first share to the previous party: " + ToString(x_vec_sh.data[0]));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] Received share from the next party: " + ToString(x_vec_next));
    Logger::DebugLog(LOC, "[P" + std::to_string(chls.party_id) + "] x_0: " + ToString(x_vec_sh.data[0]) + ", x_1: " + ToString(x_vec_sh.data[1]) + ", x_2: " + ToString(x_vec_next));
#endif
}

void ReplicatedSharing3P::RandSetup(Channels &chls) const {
    
}

void ReplicatedSharing3P::Rand(Channels &chls, uint32_t &x) const {
}

void ReplicatedSharing3P::EvaluateAdd(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const {
    z_sh.data[0] = Mod(x_sh.data[0] + y_sh.data[0], bitsize_);
    z_sh.data[1] = Mod(x_sh.data[1] + y_sh.data[1], bitsize_);
}

void ReplicatedSharing3P::EvaluateAdd(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateAdd.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    for (uint32_t i = 0; i < x_vec_sh.num_shares; ++i) {
        z_vec_sh.data[0][i] = Mod(x_vec_sh.data[0][i] + y_vec_sh.data[0][i], bitsize_);
        z_vec_sh.data[1][i] = Mod(x_vec_sh.data[1][i] + y_vec_sh.data[1][i], bitsize_);
    }
}

void ReplicatedSharing3P::EvaluateSub(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const {
    z_sh.data[0] = Mod(x_sh.data[0] - y_sh.data[0], bitsize_);
    z_sh.data[1] = Mod(x_sh.data[1] - y_sh.data[1], bitsize_);
}

void ReplicatedSharing3P::EvaluateSub(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const {
    if (x_vec_sh.num_shares != y_vec_sh.num_shares) {
        Logger::ErrorLog(LOC, "Size mismatch: x_vec_sh.num_shares != y_vec_sh.num_shares in EvaluateSub.");
        return;
    }

    if (z_vec_sh.num_shares != x_vec_sh.num_shares) {
        z_vec_sh.num_shares = x_vec_sh.num_shares;
        z_vec_sh.data[0].resize(x_vec_sh.num_shares);
        z_vec_sh.data[1].resize(x_vec_sh.num_shares);
    }

    for (uint32_t i = 0; i < x_vec_sh.num_shares; ++i) {
        z_vec_sh.data[0][i] = Mod(x_vec_sh.data[0][i] - y_vec_sh.data[0][i], bitsize_);
        z_vec_sh.data[1][i] = Mod(x_vec_sh.data[1][i] - y_vec_sh.data[1][i], bitsize_);
    }
}

void ReplicatedSharing3P::EvaluateMult(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const {
    // (t_0, t_1, t_2) forms a (3, 3)-sharing of t = x * y
    uint32_t t_sh = Mod(x_sh.data[0] * y_sh.data[0] + x_sh.data[1] * y_sh.data[0] + x_sh.data[0] * y_sh.data[1], bitsize_);


}

}    // namespace sharing
}    // namespace fsswm
