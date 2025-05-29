#ifndef SHARING_BINARY_3P_H_
#define SHARING_BINARY_3P_H_

#include <cryptoTools/Crypto/AES.h>

#include "FssWM/utils/network.h"
#include "share_types.h"

namespace fsswm {
namespace sharing {

class BinaryReplicatedSharing3P {
public:
    BinaryReplicatedSharing3P() = delete;
    explicit BinaryReplicatedSharing3P(const uint64_t bitsize);

    // Setup functions
    void OfflineSetUp(const std::string &file_path) const;
    void OnlineSetUp(const uint64_t party_id, const std::string &file_path);

    // Share data
    std::array<RepShare64, kThreeParties>       ShareLocal(const uint64_t &x) const;
    std::array<RepShareVec64, kThreeParties>    ShareLocal(const std::vector<uint64_t> &x_vec) const;
    std::array<RepShareMat64, kThreeParties>    ShareLocal(const std::vector<uint64_t> &x_mat, size_t rows, size_t cols) const;
    std::array<RepShareBlock, kThreeParties>    ShareLocal(const block &x) const;
    std::array<RepShareVecBlock, kThreeParties> ShareLocal(const std::vector<block> &x_vec_block) const;
    std::array<RepShareMatBlock, kThreeParties> ShareLocal(const std::vector<block> &x_mat_block, size_t rows, size_t cols) const;

    // Open data
    void Open(Channels &chls, const RepShare64 &x_sh, uint64_t &open_x) const;
    void Open(Channels &chls, const RepShareVec64 &x_vec_sh, std::vector<uint64_t> &open_x_vec) const;
    void Open(Channels &chls, const RepShareMat64 &x_mat_sh, std::vector<uint64_t> &open_x_mat) const;
    void Open(Channels &chls, const RepShareBlock &x_sh, block &open_x) const;
    void Open(Channels &chls, const RepShareVecBlock &x_vec_sh, std::vector<block> &open_x_vec) const;
    void Open(Channels &chls, const RepShareMatBlock &x_mat_sh, std::vector<block> &open_x_mat) const;

    // Randomness generation
    void     Rand(RepShare64 &x);
    void     Rand(RepShareBlock &x);
    uint64_t GenerateRandomValue() const;

    // Evaluation operations (addition, subtraction, multiplication, inner product)
    void EvaluateXor(const RepShare64 &x_sh, const RepShare64 &y_sh, RepShare64 &z_sh) const;
    void EvaluateXor(const RepShareVec64 &x_vec_sh, const RepShareVec64 &y_vec_sh, RepShareVec64 &z_vec_sh) const;

    void EvaluateAnd(Channels &chls, const RepShare64 &x_sh, const RepShare64 &y_sh, RepShare64 &z_sh);
    void EvaluateAnd(Channels &chls, const RepShareVec64 &x_vec_sh, const RepShareVec64 &y_vec_sh, RepShareVec64 &z_vec_sh);

    void EvaluateSelect(Channels &chls, const RepShare64 &x_sh, const RepShare64 &y_sh, const RepShare64 &c_sh, RepShare64 &z_sh);

private:
    uint64_t                          bitsize_;      /**< Bit size of the shared data */
    std::array<osuCrypto::AES, 2>     prf_;          /**< PRF for each party */
    uint64_t                          prf_idx_;      /**< Index for PRF */
    std::array<std::vector<block>, 2> prf_buff_;     /**< Buffers for PRF */
    uint64_t                          prf_buff_idx_; /**< Index for PRF buffer */

    // Internal functions
    void RandOffline(const std::string &file_path) const;
    void RandOnline(const uint64_t party_id, const std::string &file_path, uint64_t buffer_size = 256);
    void RefillBuffer();
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_BINARY_3P_H_
