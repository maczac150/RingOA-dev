#ifndef PROTOCOL_OBLIV_SELECT_H_
#define PROTOCOL_OBLIV_SELECT_H_

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"
#include "RingOA/sharing/share_types.h"

namespace ringoa {

class Channels;

namespace sharing {

class BinaryReplicatedSharing3P;
class BinarySharing2P;

}    // namespace sharing

namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace proto {

class OblivSelectParameters {
public:
    OblivSelectParameters() = delete;
    explicit OblivSelectParameters(const uint64_t d, const fss::OutputType mode = fss::OutputType::kShiftedAdditive)
        : params_(d, 1, fss::kOptimizedEvalType, mode) {
    }

    void ReconfigureParameters(const uint64_t d, const fss::OutputType mode = fss::OutputType::kShiftedAdditive) {
        params_.ReconfigureParameters(d, 1, fss::kOptimizedEvalType, mode);
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

struct OblivSelectKey {
    uint64_t         party_id;
    fss::dpf::DpfKey prev_key;
    fss::dpf::DpfKey next_key;
    uint64_t         prev_r_sh;
    uint64_t         next_r_sh;
    uint64_t         r;
    uint64_t         r_sh_0;
    uint64_t         r_sh_1;

    OblivSelectKey() = delete;
    OblivSelectKey(const uint64_t id, const OblivSelectParameters &params);
    ~OblivSelectKey() = default;

    OblivSelectKey(const OblivSelectKey &)            = delete;
    OblivSelectKey &operator=(const OblivSelectKey &) = delete;
    OblivSelectKey(OblivSelectKey &&)                 = default;
    OblivSelectKey &operator=(OblivSelectKey &&)      = default;

    bool operator==(const OblivSelectKey &rhs) const {
        return (party_id == rhs.party_id) && (prev_key == rhs.prev_key) && (next_key == rhs.next_key) &&
               (prev_r_sh == rhs.prev_r_sh) && (next_r_sh == rhs.next_r_sh) &&
               (r == rhs.r) && (r_sh_0 == rhs.r_sh_0) && (r_sh_1 == rhs.r_sh_1);
    }
    bool operator!=(const OblivSelectKey &rhs) const {
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
    OblivSelectParameters params_;
    size_t                serialized_size_;
};

class OblivSelectKeyGenerator {
public:
    OblivSelectKeyGenerator() = delete;
    OblivSelectKeyGenerator(const OblivSelectParameters &params,
                            sharing::BinarySharing2P    &bss);

    std::array<OblivSelectKey, 3> GenerateKeys() const;

private:
    OblivSelectParameters     params_;
    fss::dpf::DpfKeyGenerator gen_;
    sharing::BinarySharing2P &bss_;
};

class OblivSelectEvaluator {
public:
    OblivSelectEvaluator() = delete;
    OblivSelectEvaluator(const OblivSelectParameters        &params,
                         sharing::BinaryReplicatedSharing3P &brss);

    void Evaluate(Channels                         &chls,
                  const OblivSelectKey             &key,
                  const sharing::RepShareViewBlock &database,
                  const sharing::RepShare64        &index,
                  sharing::RepShareBlock           &result) const;

    void Evaluate(Channels                      &chls,
                  const OblivSelectKey          &key,
                  std::vector<block>            &uv_prev,
                  std::vector<block>            &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const OblivSelectKey          &key1,
                           const OblivSelectKey          &key2,
                           std::vector<block>            &uv_prev,
                           std::vector<block>            &uv_next,
                           const sharing::RepShareView64 &database,
                           const sharing::RepShareVec64  &index,
                           sharing::RepShareVec64        &result) const;

    block ComputeDotProductBlockSIMD(const fss::dpf::DpfKey       &key,
                                     const std::span<const block> &database,
                                     const uint64_t                pr) const;

    std::pair<uint64_t, uint64_t> EvaluateFullDomainThenDotProduct(const fss::dpf::DpfKey        &key_prev,
                                                                   const fss::dpf::DpfKey        &key_next,
                                                                   std::vector<block>            &uv_prev,
                                                                   std::vector<block>            &uv_next,
                                                                   const sharing::RepShareView64 &database,
                                                                   const uint64_t                 pr_prev,
                                                                   const uint64_t                 pr_next) const;

private:
    OblivSelectParameters               params_;
    fss::dpf::DpfEvaluator              eval_;
    sharing::BinaryReplicatedSharing3P &brss_;
    fss::prg::PseudoRandomGenerator    &G_;

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructPRBinary(Channels                  &chls,
                                                      const OblivSelectKey      &key,
                                                      const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructPRBinary(Channels                     &chls,
                                                const OblivSelectKey         &key1,
                                                const OblivSelectKey         &key2,
                                                const sharing::RepShareVec64 &index) const;

    void EvaluateNextSeed(const uint64_t current_level, const block &current_seed, const bool &current_control_bit,
                          std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
                          const fss::dpf::DpfKey &key) const;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_OBLIV_SELECT_H_
