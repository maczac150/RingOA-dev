#ifndef PROTOCOL_ZERO_TEST_H_
#define PROTOCOL_ZERO_TEST_H_

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

class ZeroTestParameters {
public:
    ZeroTestParameters() = delete;
    explicit ZeroTestParameters(const uint64_t n, const uint64_t e)
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

struct ZeroTestKey {
    fss::dpf::DpfKey dpf_key;
    uint64_t         shr_in;

    ZeroTestKey() = delete;
    explicit ZeroTestKey(const uint64_t party_id, const ZeroTestParameters &params);
    ~ZeroTestKey() = default;

    ZeroTestKey(const ZeroTestKey &)                = delete;
    ZeroTestKey &operator=(const ZeroTestKey &)     = delete;
    ZeroTestKey(ZeroTestKey &&) noexcept            = default;
    ZeroTestKey &operator=(ZeroTestKey &&) noexcept = default;

    bool operator==(const ZeroTestKey &rhs) const {
        return (dpf_key == rhs.dpf_key) &&
               (shr_in == rhs.shr_in);
    }
    bool operator!=(const ZeroTestKey &rhs) const {
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
    ZeroTestParameters params_;
    size_t             serialized_size_;
};

class ZeroTestKeyGenerator {
public:
    ZeroTestKeyGenerator() = delete;
    explicit ZeroTestKeyGenerator(
        const ZeroTestParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    std::pair<ZeroTestKey, ZeroTestKey> GenerateKeys() const;

private:
    ZeroTestParameters          params_;
    fss::dpf::DpfKeyGenerator   gen_;
    sharing::AdditiveSharing2P &ss_in_;
    sharing::AdditiveSharing2P &ss_out_;
};

class ZeroTestEvaluator {
public:
    ZeroTestEvaluator() = delete;
    explicit ZeroTestEvaluator(
        const ZeroTestParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const ZeroTestKey &key, const uint64_t x) const;
    uint64_t EvaluateMaskedInput(const ZeroTestKey &key, const uint64_t x) const;

private:
    ZeroTestParameters          params_;
    fss::dpf::DpfEvaluator      eval_;
    sharing::AdditiveSharing2P &ss_in_;
    sharing::AdditiveSharing2P &ss_out_;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_ZERO_TEST_H_
