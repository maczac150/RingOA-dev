#ifndef SHARING_ADDITIVE_3P_H_
#define SHARING_ADDITIVE_3P_H_

#include "cryptoTools/Network/Channel.h"
#include "share3_types.h"

namespace fsswm {
namespace sharing {

constexpr size_t kNumParties = 3;

struct Channels {
    uint32_t    party_id;
    oc::Channel prev;
    oc::Channel next;

    Channels(const uint32_t party_id, oc::Channel &prev, oc::Channel &next)
        : party_id(party_id), prev(prev), next(next) {
    }
};

class ReplicatedSharing3P {
public:
    ReplicatedSharing3P() = delete;
    explicit ReplicatedSharing3P(const uint32_t bitsize);

    void OfflineSetUp(Channels &chls, const std::string &file_path) const;

    std::array<SharePair, kNumParties>  ShareLocal(const uint32_t &x) const;
    std::array<SharesPair, kNumParties> ShareLocal(const std::vector<uint32_t> &x_vec) const;

    void Open(Channels &chls, const SharePair &x_sh, uint32_t &open_x) const;
    void Open(Channels &chls, const SharesPair &x_vec_sh, std::vector<uint32_t> &open_x_vec) const;

    void RandSetup(Channels &chls) const;
    void Rand(Channels &chls, uint32_t &x) const;

    void EvaluateAdd(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const;
    void EvaluateAdd(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const;

    void EvaluateSub(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const;
    void EvaluateSub(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const;

    void EvaluateMult(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const;

    void EvaluateInnerProduct(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, uint32_t &z) const;

private:
    uint32_t bitsize_; /**< Bit size of the shared data */
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_ADDITIVE_3P_H_
