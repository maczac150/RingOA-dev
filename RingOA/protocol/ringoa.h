#ifndef PROTOCOL_RINGOA_H_
#define PROTOCOL_RINGOA_H_

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

class RingOaParameters {
public:
    RingOaParameters() = delete;
    explicit RingOaParameters(const uint64_t d)
        : params_(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive) {
    }

    void ReconfigureParameters(const uint64_t d) {
        params_.ReconfigureParameters(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive);
    }

    uint64_t GetDatabaseSize() const {
        return params_.GetInputBitsize();
    }

    const fss::dpf::DpfParameters GetParameters() const {
        return params_;
    }
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }
    void PrintParameters() const;

private:
    fss::dpf::DpfParameters params_;
};

struct RingOaKey {
    uint64_t         party_id;
    fss::dpf::DpfKey key_from_prev;
    fss::dpf::DpfKey key_from_next;
    uint64_t         rsh_from_prev;
    uint64_t         rsh_from_next;
    uint64_t         wsh_from_prev;
    uint64_t         wsh_from_next;

    RingOaKey() = delete;
    RingOaKey(const uint64_t id, const RingOaParameters &params);
    ~RingOaKey() = default;

    RingOaKey(const RingOaKey &)            = delete;
    RingOaKey &operator=(const RingOaKey &) = delete;
    RingOaKey(RingOaKey &&)                 = default;
    RingOaKey &operator=(RingOaKey &&)      = default;

    bool operator==(const RingOaKey &rhs) const {
        return (party_id == rhs.party_id) && (key_from_prev == rhs.key_from_prev) && (key_from_next == rhs.key_from_next) &&
               (rsh_from_prev == rhs.rsh_from_prev) && (rsh_from_next == rhs.rsh_from_next) &&
               (wsh_from_prev == rhs.wsh_from_prev) && (wsh_from_next == rhs.wsh_from_next);
    }
    bool operator!=(const RingOaKey &rhs) const {
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
    RingOaParameters params_;
    size_t           serialized_size_;
};

class RingOaKeyGenerator {
public:
    RingOaKeyGenerator() = delete;
    RingOaKeyGenerator(const RingOaParameters     &params,
                       sharing::AdditiveSharing2P &ass);

    void OfflineSetUp(const uint64_t num_selection, const std::string &file_path) const;

    std::array<RingOaKey, 3> GenerateKeys() const;

private:
    RingOaParameters            params_;
    fss::dpf::DpfKeyGenerator   gen_;
    sharing::AdditiveSharing2P &ass_;
};

class RingOaEvaluator {
public:
    RingOaEvaluator() = delete;

    RingOaEvaluator(const RingOaParameters       &params,
                    sharing::ReplicatedSharing3P &rss,
                    sharing::AdditiveSharing2P   &ass_prev,
                    sharing::AdditiveSharing2P   &ass_next);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path) const;

    void Evaluate(Channels                      &chls,
                  const RingOaKey               &key,
                  std::vector<block>            &uv_prev,
                  std::vector<block>            &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const RingOaKey               &key1,
                           const RingOaKey               &key2,
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
    RingOaParameters              params_;
    fss::dpf::DpfEvaluator        eval_;
    sharing::ReplicatedSharing3P &rss_;
    sharing::AdditiveSharing2P   &ass_prev_;
    sharing::AdditiveSharing2P   &ass_next_;

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructMaskedValue(
        Channels                  &chls,
        const RingOaKey           &key,
        const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructMaskedValue(
        Channels                     &chls,
        const RingOaKey              &key1,
        const RingOaKey              &key2,
        const sharing::RepShareVec64 &index) const;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_RINGOA_H_
