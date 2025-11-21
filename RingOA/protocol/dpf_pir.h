#ifndef PROTOCOL_DPF_PIR_H_
#define PROTOCOL_DPF_PIR_H_

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"
#include "RingOA/sharing/share_types.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace proto {

class DpfPirParameters {
public:
    DpfPirParameters() = delete;
    explicit DpfPirParameters(const uint64_t        d,
                              const uint64_t        dpf_out = 1,
                              const fss::EvalType   type    = fss::kOptimizedEvalType,
                              const fss::OutputType mode    = fss::OutputType::kShiftedAdditive)
        : params_(d, dpf_out, type, mode) {
    }

    void ReconfigureParameters(const uint64_t        d,
                               const uint64_t        dpf_out = 1,
                               const fss::EvalType   type    = fss::kOptimizedEvalType,
                               const fss::OutputType mode    = fss::OutputType::kShiftedAdditive) {
        params_.ReconfigureParameters(d, dpf_out, type, mode);
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

struct DpfPirKey {
    fss::dpf::DpfKey dpf_key;
    uint64_t         r_sh;
    uint64_t         w_sh;

    DpfPirKey() = delete;
    DpfPirKey(const uint64_t id, const DpfPirParameters &params);
    ~DpfPirKey() = default;

    DpfPirKey(const DpfPirKey &)            = delete;
    DpfPirKey &operator=(const DpfPirKey &) = delete;
    DpfPirKey(DpfPirKey &&)                 = default;
    DpfPirKey &operator=(DpfPirKey &&)      = default;

    bool operator==(const DpfPirKey &rhs) const {
        return (dpf_key == rhs.dpf_key) && (r_sh == rhs.r_sh) && (w_sh == rhs.w_sh);
    }
    bool operator!=(const DpfPirKey &rhs) const {
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
    DpfPirParameters params_;
    size_t           serialized_size_;
};

class DpfPirKeyGenerator {
public:
    DpfPirKeyGenerator() = delete;
    DpfPirKeyGenerator(const DpfPirParameters     &params,
                       sharing::AdditiveSharing2P &ss);

    void OfflineSetUp(const uint64_t num_access, const std::string &file_path) const;

    std::pair<DpfPirKey, DpfPirKey> GenerateKeys() const;

private:
    DpfPirParameters            params_;
    fss::dpf::DpfKeyGenerator   gen_;
    sharing::AdditiveSharing2P &ss_;
};

class DpfPirEvaluator {
public:
    DpfPirEvaluator() = delete;
    DpfPirEvaluator(const DpfPirParameters &params, sharing::AdditiveSharing2P &ss);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path) const;

    uint64_t EvaluateSharedIndex(osuCrypto::Channel          &chl,
                                 const DpfPirKey             &key,
                                 std::vector<block>          &uv,
                                 const std::vector<uint64_t> &database,
                                 const uint64_t               index_sh) const;

    uint64_t EvaluateMaskedIndex(osuCrypto::Channel          &chl,
                                 const DpfPirKey             &key,
                                 std::vector<block>          &uv,
                                 const std::vector<uint64_t> &database,
                                 const uint64_t               masked_index) const;

    void EvaluateMaskedIndex_Double(osuCrypto::Channel          &chl,
                                    const DpfPirKey             &key,
                                    std::vector<block>          &uv,
                                    const std::vector<uint64_t> &database_1,
                                    const std::vector<uint64_t> &database_2,
                                    const uint64_t               masked_index,
                                    std::array<uint64_t, 2>     &dot_product) const;

    void EvaluateMaskedIndex_Triple(osuCrypto::Channel          &chl,
                                    const DpfPirKey             &key,
                                    std::vector<block>          &uv,
                                    const std::vector<uint64_t> &database_1,
                                    const std::vector<uint64_t> &database_2,
                                    const std::vector<uint64_t> &database_3,
                                    const uint64_t               masked_index,
                                    std::array<uint64_t, 3>     &dot_product) const;

    uint64_t EvaluateFullDomainThenDotProduct(const fss::dpf::DpfKey      &key,
                                              const std::vector<uint64_t> &database,
                                              const uint64_t               masked_idx,
                                              std::vector<block>          &outputs) const;

    void EvaluateFullDomainThenDotProduct_Double(const fss::dpf::DpfKey      &key,
                                                 const std::vector<uint64_t> &database_1,
                                                 const std::vector<uint64_t> &database_2,
                                                 const uint64_t               masked_idx,
                                                 std::vector<block>          &outputs,
                                                 std::array<uint64_t, 2>     &dot_product) const;

    void EvaluateFullDomainThenDotProduct_Triple(const fss::dpf::DpfKey      &key,
                                                 const std::vector<uint64_t> &database_1,
                                                 const std::vector<uint64_t> &database_2,
                                                 const std::vector<uint64_t> &database_3,
                                                 const uint64_t               masked_idx,
                                                 std::vector<block>          &outputs,
                                                 std::array<uint64_t, 3>     &dot_product) const;

    void EvaluateFullDomainThenDotProduct_Vectorized(const fss::dpf::DpfKey                   &key,
                                                     const std::vector<std::vector<uint64_t>> &databases,
                                                     const uint64_t                            masked_idx,
                                                     std::vector<block>                       &outputs,
                                                     std::vector<uint64_t>                    &dot_product) const;

    uint64_t EvaluateSharedIndexNaive(osuCrypto::Channel          &chl,
                                      const DpfPirKey             &key,
                                      std::vector<uint64_t>       &uv,
                                      const std::vector<uint64_t> &database,
                                      const uint64_t               index_sh) const;

    uint64_t EvaluateMaskedIndexNaive(osuCrypto::Channel          &chl,
                                      const DpfPirKey             &key,
                                      std::vector<uint64_t>       &uv,
                                      const std::vector<uint64_t> &database,
                                      const uint64_t               masked_index) const;

private:
    DpfPirParameters                 params_;
    fss::dpf::DpfEvaluator           eval_;
    sharing::AdditiveSharing2P      &ss_;
    fss::prg::PseudoRandomGenerator &G_;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_DPF_PIR_H_
