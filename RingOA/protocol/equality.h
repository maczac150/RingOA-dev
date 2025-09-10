#ifndef PROTOCOL_EQUALITY_H_
#define PROTOCOL_EQUALITY_H_

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

class EqualityParameters {
public:
    EqualityParameters() = delete;
    explicit EqualityParameters(const uint64_t n, const uint64_t e)
        : params_(n, e) {
    }

    uint64_t GetInputBitsize() const {
        return params_.GetInputBitsize();
    }
    uint64_t GetOutputBitsize() const {
        return params_.GetOutputBitsize();
    }

    void ReconfigureParameters(const uint64_t n, const uint64_t e) {
        params_.ReconfigureParameters(n, e);
    }

    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }
    const fss::dpf::DpfParameters &GetParameters() const {
        return params_;
    }
    void PrintParameters() const;

private:
    fss::dpf::DpfParameters params_;
};

struct EqualityKey {
    fss::dpf::DpfKey dpf_key;
    uint64_t         shr1_in;
    uint64_t         shr2_in;

    EqualityKey() = delete;
    explicit EqualityKey(const uint64_t party_id, const EqualityParameters &params);
    ~EqualityKey() = default;

    EqualityKey(const EqualityKey &)                = delete;
    EqualityKey &operator=(const EqualityKey &)     = delete;
    EqualityKey(EqualityKey &&) noexcept            = default;
    EqualityKey &operator=(EqualityKey &&) noexcept = default;

    bool operator==(const EqualityKey &rhs) const {
        return (dpf_key == rhs.dpf_key) &&
               (shr1_in == rhs.shr1_in) &&
               (shr2_in == rhs.shr2_in);
    }
    bool operator!=(const EqualityKey &rhs) const {
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
    EqualityParameters params_;
    size_t             serialized_size_;
};

class EqualityKeyGenerator {
public:
    EqualityKeyGenerator() = delete;
    explicit EqualityKeyGenerator(
        const EqualityParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    std::pair<EqualityKey, EqualityKey> GenerateKeys() const;

private:
    EqualityParameters          params_;
    fss::dpf::DpfKeyGenerator   gen_;
    sharing::AdditiveSharing2P &ss_in_;
    sharing::AdditiveSharing2P &ss_out_;
};

class EqualityEvaluator {
public:
    EqualityEvaluator() = delete;
    explicit EqualityEvaluator(
        const EqualityParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const EqualityKey &key, const uint64_t x1, const uint64_t x2) const;
    uint64_t EvaluateMaskedInput(const EqualityKey &key, const uint64_t x1, const uint64_t x2) const;

private:
    EqualityParameters          params_;
    fss::dpf::DpfEvaluator      eval_;
    sharing::AdditiveSharing2P &ss_in_;
    sharing::AdditiveSharing2P &ss_out_;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_EQUALITY_H_
