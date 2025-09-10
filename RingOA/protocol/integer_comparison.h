#ifndef PROTOCOL_INTEGER_COMPARISON_H_
#define PROTOCOL_INTEGER_COMPARISON_H_

#include "ddcf.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

class IntegerComparisonParameters {
public:
    IntegerComparisonParameters() = delete;
    explicit IntegerComparisonParameters(const uint64_t n, const uint64_t e)
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
    const DdcfParameters &GetParameters() const {
        return params_;
    }
    void PrintParameters() const;

private:
    DdcfParameters params_;
};

struct IntegerComparisonKey {
    DdcfKey  ddcf_key;
    uint64_t shr1_in;
    uint64_t shr2_in;

    IntegerComparisonKey() = delete;
    explicit IntegerComparisonKey(const uint64_t party_id, const IntegerComparisonParameters &params);
    ~IntegerComparisonKey() = default;

    IntegerComparisonKey(const IntegerComparisonKey &)                = delete;
    IntegerComparisonKey &operator=(const IntegerComparisonKey &)     = delete;
    IntegerComparisonKey(IntegerComparisonKey &&) noexcept            = default;
    IntegerComparisonKey &operator=(IntegerComparisonKey &&) noexcept = default;

    bool operator==(const IntegerComparisonKey &rhs) const {
        return (ddcf_key == rhs.ddcf_key) &&
               (shr1_in == rhs.shr1_in) &&
               (shr2_in == rhs.shr2_in);
    }
    bool operator!=(const IntegerComparisonKey &rhs) const {
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
    IntegerComparisonParameters params_;
    size_t                      serialized_size_;
};

class IntegerComparisonKeyGenerator {
public:
    IntegerComparisonKeyGenerator() = delete;
    explicit IntegerComparisonKeyGenerator(
        const IntegerComparisonParameters &params,
        sharing::AdditiveSharing2P        &ss_in,
        sharing::AdditiveSharing2P        &ss_out);

    std::pair<IntegerComparisonKey, IntegerComparisonKey> GenerateKeys() const;

private:
    IntegerComparisonParameters params_;
    DdcfKeyGenerator            gen_;
    sharing::AdditiveSharing2P &ss_in_;
    sharing::AdditiveSharing2P &ss_out_;
};

class IntegerComparisonEvaluator {
public:
    IntegerComparisonEvaluator() = delete;

    explicit IntegerComparisonEvaluator(
        const IntegerComparisonParameters &params,
        sharing::AdditiveSharing2P        &ss_in,
        sharing::AdditiveSharing2P        &ss_out);

    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const IntegerComparisonKey &key, const uint64_t x1, const uint64_t x2) const;
    uint64_t EvaluateMaskedInput(const IntegerComparisonKey &key, const uint64_t x1, const uint64_t x2) const;

private:
    IntegerComparisonParameters params_;
    DdcfEvaluator               eval_;
    sharing::AdditiveSharing2P &ss_in_;
    sharing::AdditiveSharing2P &ss_out_;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_INTEGER_COMPARISON_H_
