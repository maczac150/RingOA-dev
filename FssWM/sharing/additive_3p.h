#ifndef SHARING_ADDITIVE_3P_H_
#define SHARING_ADDITIVE_3P_H_

#include "cryptoTools/Crypto/AES.h"
#include "share3_types.h"

namespace fsswm {
namespace sharing {

class ReplicatedSharing3P {
public:
    ReplicatedSharing3P() = delete;
    explicit ReplicatedSharing3P(const uint32_t bitsize);

    // Setup functions
    void OfflineSetUp(const std::string &file_path) const;
    void OnlineSetUp(const uint32_t party_id, const std::string &file_path);

    // Share data
    std::array<RepShare, kNumParties>    ShareLocal(const uint32_t &x) const;
    std::array<RepShareVec, kNumParties> ShareLocal(const UIntVec &x_vec) const;
    std::array<RepShareMat, kNumParties> ShareLocal(const UIntMat &x_mat) const;

    // Open data
    void Open(Channels &chls, const RepShare &x_sh, uint32_t &open_x) const;
    void Open(Channels &chls, const RepShareVec &x_vec_sh, UIntVec &open_x_vec) const;
    void Open(Channels &chls, const RepShareMat &x_mat_sh, UIntMat &open_x_mat) const;

    // Randomness generation
    void     Rand(RepShare &x);
    uint32_t GenerateRandomValue() const;

    // Evaluation operations (addition, subtraction, multiplication, inner product)
    void EvaluateAdd(const RepShare &x_sh, const RepShare &y_sh, RepShare &z_sh) const;
    void EvaluateAdd(const RepShareVec &x_vec_sh, const RepShareVec &y_vec_sh, RepShareVec &z_vec_sh) const;

    void EvaluateSub(const RepShare &x_sh, const RepShare &y_sh, RepShare &z_sh) const;
    void EvaluateSub(const RepShareVec &x_vec_sh, const RepShareVec &y_vec_sh, RepShareVec &z_vec_sh) const;

    void EvaluateMult(Channels &chls, const RepShare &x_sh, const RepShare &y_sh, RepShare &z_sh);
    void EvaluateMult(Channels &chls, const RepShareVec &x_vec_sh, const RepShareVec &y_vec_sh, RepShareVec &z_vec_sh);

    void EvaluateInnerProduct(Channels &chls, const RepShareVec &x_vec_sh, const RepShareVec &y_vec_sh, RepShare &z);

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
