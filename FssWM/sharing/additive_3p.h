#ifndef SHARING_ADDITIVE_3P_H_
#define SHARING_ADDITIVE_3P_H_

#include "cryptoTools/Crypto/AES.h"
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

    // Setup functions
    void OfflineSetUp(const std::string &file_path) const;
    void OnlineSetUp(const uint32_t party_id, const std::string &file_path);

    // Share data
    std::array<SharePair, kNumParties>  ShareLocal(const uint32_t &x) const;
    std::array<SharesPair, kNumParties> ShareLocal(const std::vector<uint32_t> &x_vec) const;

    // Open data
    void Open(Channels &chls, const SharePair &x_sh, uint32_t &open_x) const;
    void Open(Channels &chls, const SharesPair &x_vec_sh, std::vector<uint32_t> &open_x_vec) const;

    // Randomness generation
    void Rand(SharePair &x);
    uint32_t GenerateRandomValue() const;

    // Evaluation operations (addition, subtraction, multiplication, inner product)
    void EvaluateAdd(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const;
    void EvaluateAdd(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const;

    void EvaluateSub(const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh) const;
    void EvaluateSub(const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh) const;

    void EvaluateMult(Channels &chls, const SharePair &x_sh, const SharePair &y_sh, SharePair &z_sh);
    void EvaluateMult(Channels &chls, const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharesPair &z_vec_sh);

    void EvaluateInnerProduct(Channels &chls, const SharesPair &x_vec_sh, const SharesPair &y_vec_sh, SharePair &z);

private:
    uint32_t                              bitsize_;      /**< Bit size of the shared data */
    std::array<oc::AES, 2>                prf_;          /**< PRF for each party */
    uint32_t                              prf_idx_;      /**< Index for PRF */
    std::array<std::vector<oc::block>, 2> prf_buff_;     /**< Buffers for PRF */
    uint32_t                              prf_buff_idx_; /**< Index for PRF buffer */

    // Internal functions
    void RandOffline(const std::string &file_path) const;
    void RandOnline(const uint32_t party_id, const std::string &file_path, uint32_t buffer_size = 256);
    void RefillBuffer();
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_ADDITIVE_3P_H_
