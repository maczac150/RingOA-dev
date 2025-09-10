#ifndef PROTOCOL_SHARED_OT_H_
#define PROTOCOL_SHARED_OT_H_

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

namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace proto {

class SharedOtParameters {
public:
    SharedOtParameters() = delete;
    explicit SharedOtParameters(const uint64_t d, const fss::EvalType type = fss::kOptimizedEvalType)
        : params_(d, d, type, fss::OutputType::kShiftedAdditive) {
    }

    void ReconfigureParameters(const uint64_t d, const fss::EvalType type = fss::kOptimizedEvalType) {
        params_.ReconfigureParameters(d, d, type, fss::OutputType::kShiftedAdditive);
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

struct SharedOtKey {
    uint64_t         party_id;
    fss::dpf::DpfKey key_from_prev;
    fss::dpf::DpfKey key_from_next;
    uint64_t         rsh_from_prev;
    uint64_t         rsh_from_next;

    SharedOtKey() = delete;
    SharedOtKey(const uint64_t id, const SharedOtParameters &params);
    ~SharedOtKey() = default;

    SharedOtKey(const SharedOtKey &)            = delete;
    SharedOtKey &operator=(const SharedOtKey &) = delete;
    SharedOtKey(SharedOtKey &&)                 = default;
    SharedOtKey &operator=(SharedOtKey &&)      = default;

    bool operator==(const SharedOtKey &rhs) const {
        return (party_id == rhs.party_id) && (key_from_prev == rhs.key_from_prev) && (key_from_next == rhs.key_from_next) &&
               (rsh_from_prev == rhs.rsh_from_prev) && (rsh_from_next == rhs.rsh_from_next);
    }
    bool operator!=(const SharedOtKey &rhs) const {
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
    SharedOtParameters params_;
    size_t             serialized_size_;
};

class SharedOtKeyGenerator {
public:
    SharedOtKeyGenerator() = delete;
    SharedOtKeyGenerator(const SharedOtParameters   &params,
                         sharing::AdditiveSharing2P &ass);

    std::array<SharedOtKey, 3> GenerateKeys() const;

private:
    SharedOtParameters params_;
    fss::dpf::DpfKeyGenerator gen_;
    sharing::AdditiveSharing2P   &ass_;
};

class SharedOtEvaluator {
public:
    SharedOtEvaluator() = delete;
    SharedOtEvaluator(const SharedOtParameters     &params,
                      sharing::ReplicatedSharing3P &rss);

    void Evaluate(Channels                      &chls,
                  const SharedOtKey             &key,
                  std::vector<uint64_t>         &uv_prev,
                  std::vector<uint64_t>         &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const SharedOtKey             &key1,
                           const SharedOtKey             &key2,
                           std::vector<uint64_t>         &uv_prev,
                           std::vector<uint64_t>         &uv_next,
                           const sharing::RepShareView64 &database,
                           const sharing::RepShareVec64  &index,
                           sharing::RepShareVec64        &result) const;

    std::pair<uint64_t, uint64_t> EvaluateFullDomainThenDotProduct(
        const uint64_t                 party_id,
        const fss::dpf::DpfKey        &key_from_prev,
        const fss::dpf::DpfKey        &key_from_next,
        std::vector<uint64_t>         &uv_prev,
        std::vector<uint64_t>         &uv_next,
        const sharing::RepShareView64 &database,
        const uint64_t                 pr_prev,
        const uint64_t                 pr_next) const;

private:
    SharedOtParameters               params_;
    fss::dpf::DpfEvaluator           eval_;
    sharing::ReplicatedSharing3P    &rss_;
    fss::prg::PseudoRandomGenerator &G_;

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructMaskedValue(
        Channels                  &chls,
        const SharedOtKey         &key,
        const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructMaskedValue(
        Channels                     &chls,
        const SharedOtKey            &key1,
        const SharedOtKey            &key2,
        const sharing::RepShareVec64 &index) const;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_SHARED_OT_H_
