#ifndef PROTOCOL_RINGOA_FSC_H_
#define PROTOCOL_RINGOA_FSC_H_

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"
#include "RingOA/sharing/share_types.h"

namespace ringoa {

class Channels;

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

class RingOaFscParameters {
public:
    RingOaFscParameters() = delete;
    explicit RingOaFscParameters(const uint64_t d)
        : params_(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive),
          db_bitsize_(d),
          share_bitsize_(d) {
    }
    explicit RingOaFscParameters(const uint64_t d, const uint64_t s)
        : params_(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive),
          db_bitsize_(d),
          share_bitsize_(s) {
    }

    void ReconfigureParameters(const uint64_t d) {
        params_.ReconfigureParameters(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive);
        db_bitsize_    = d;
        share_bitsize_ = d;
    }
    void ReconfigureParameters(const uint64_t d, const uint64_t s) {
        params_.ReconfigureParameters(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive);
        db_bitsize_    = d;
        share_bitsize_ = s;
    }

    uint64_t GetDatabaseSize() const {
        return db_bitsize_;
    }
    uint64_t GetShareSize() const {
        return share_bitsize_;
    }

    const fss::dpf::DpfParameters GetParameters() const {
        return params_;
    }
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo() + ", DB size: " + std::to_string(db_bitsize_) + ", Share size: " + std::to_string(share_bitsize_);
    }
    void PrintParameters() const;

private:
    fss::dpf::DpfParameters params_;
    uint64_t                db_bitsize_;
    uint64_t                share_bitsize_;
};

struct RingOaFscKey {
    uint64_t         party_id;
    fss::dpf::DpfKey key_from_prev;
    fss::dpf::DpfKey key_from_next;
    uint64_t         rsh_from_prev;
    uint64_t         rsh_from_next;
    uint64_t         w_from_prev;
    uint64_t         w_from_next;

    RingOaFscKey() = delete;
    RingOaFscKey(const uint64_t id, const RingOaFscParameters &params);
    ~RingOaFscKey() = default;

    RingOaFscKey(const RingOaFscKey &)            = delete;
    RingOaFscKey &operator=(const RingOaFscKey &) = delete;
    RingOaFscKey(RingOaFscKey &&)                 = default;
    RingOaFscKey &operator=(RingOaFscKey &&)      = default;

    bool operator==(const RingOaFscKey &rhs) const {
        return (party_id == rhs.party_id) && (key_from_prev == rhs.key_from_prev) && (key_from_next == rhs.key_from_next) &&
               (rsh_from_prev == rhs.rsh_from_prev) && (rsh_from_next == rhs.rsh_from_next) &&
               (w_from_prev == rhs.w_from_prev) && (w_from_next == rhs.w_from_next);
    }
    bool operator!=(const RingOaFscKey &rhs) const {
        return !(*this == rhs);
    }

    size_t GetSerializedSize() const {
        return serialized_size_;
    }
    size_t CalculateSerializedSize() const;

    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);

    void PrintKey(const bool detailed = false) const;

private:
    RingOaFscParameters params_;
    size_t              serialized_size_;
};

class RingOaFscKeyGenerator {
public:
    RingOaFscKeyGenerator() = delete;
    RingOaFscKeyGenerator(const RingOaFscParameters    &params,
                          sharing::ReplicatedSharing3P &rss,
                          sharing::AdditiveSharing2P   &ass);

    void GenerateDatabaseShare(const std::vector<uint64_t>           &database,
                               std::array<sharing::RepShareVec64, 3> &db_sh,
                               std::array<bool, 3>                   &v_sign) const;

    void GenerateDatabaseShare(const std::vector<uint64_t>           &database,
                               std::array<sharing::RepShareMat64, 3> &db_sh,
                               size_t                                 rows,
                               size_t                                 cols,
                               std::array<bool, 3>                   &v_sign) const;

    std::array<RingOaFscKey, 3> GenerateKeys(std::array<bool, 3> &v_sign) const;

private:
    RingOaFscParameters           params_;
    fss::dpf::DpfKeyGenerator     gen_;
    sharing::ReplicatedSharing3P &rss_;
    sharing::AdditiveSharing2P   &ass_;

    uint64_t ComputeSignCorrection(
        block   &final_seed_0,
        block   &final_seed_1,
        bool     final_control_bit_1,
        bool     v_sign,
        uint64_t alpha_hat) const;
};

class RingOaFscEvaluator {
public:
    RingOaFscEvaluator() = delete;

    RingOaFscEvaluator(const RingOaFscParameters    &params,
                       sharing::ReplicatedSharing3P &rss,
                       sharing::AdditiveSharing2P   &ass_prev,
                       sharing::AdditiveSharing2P   &ass_next);

    void Evaluate(Channels                      &chls,
                  const RingOaFscKey            &key,
                  std::vector<block>            &uv_prev,
                  std::vector<block>            &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const RingOaFscKey            &key1,
                           const RingOaFscKey            &key2,
                           std::vector<block>            &uv_prev,
                           std::vector<block>            &uv_next,
                           const sharing::RepShareView64 &database,
                           const sharing::RepShareVec64  &index,
                           sharing::RepShareVec64        &result) const;

    std::pair<uint64_t, uint64_t> EvaluateFullDomainThenDotProduct(
        const uint64_t                 party_id,
        const fss::dpf::DpfKey        &key_from_prev,
        const fss::dpf::DpfKey        &key_from_next,
        std::vector<block>            &uv_prev,
        std::vector<block>            &uv_next,
        const sharing::RepShareView64 &database,
        const uint64_t                 pr_prev,
        const uint64_t                 pr_next) const;

private:
    RingOaFscParameters           params_;
    fss::dpf::DpfEvaluator        eval_;
    sharing::ReplicatedSharing3P &rss_;
    sharing::AdditiveSharing2P   &ass_prev_;
    sharing::AdditiveSharing2P   &ass_next_;

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructMaskedValue(
        Channels                  &chls,
        const RingOaFscKey        &key,
        const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructMaskedValue(
        Channels                     &chls,
        const RingOaFscKey           &key1,
        const RingOaFscKey           &key2,
        const sharing::RepShareVec64 &index) const;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_RINGOA_FSC_H_
