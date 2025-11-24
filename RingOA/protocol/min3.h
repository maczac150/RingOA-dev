#ifndef PROTOCOL_MIN3_H_
#define PROTOCOL_MIN3_H_

#include "integer_comparison.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

class Min3Parameters {
public:
    Min3Parameters() = delete;
    explicit Min3Parameters(const uint64_t n)
        : params_(n + 1, n) {
    }

    uint64_t GetInputBitsize() const {
        return params_.GetInputBitsize();
    }
    uint64_t GetOutputBitsize() const {
        return params_.GetOutputBitsize();
    }

    void ReconfigureParameters(const uint64_t n) {
        params_.ReconfigureParameters(n + 1, n);
    }

    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }
    const IntegerComparisonParameters &GetParameters() const {
        return params_;
    }
    void PrintParameters() const;

private:
    IntegerComparisonParameters params_;
};

struct Min3Key {
    IntegerComparisonKey ic_key_1;
    IntegerComparisonKey ic_key_2;

    Min3Key() = delete;
    explicit Min3Key(const uint64_t party_id, const Min3Parameters &params);
    ~Min3Key() = default;

    Min3Key(const Min3Key &)                = delete;
    Min3Key &operator=(const Min3Key &)     = delete;
    Min3Key(Min3Key &&) noexcept            = default;
    Min3Key &operator=(Min3Key &&) noexcept = default;

    bool operator==(const Min3Key &rhs) const {
        return (ic_key_1 == rhs.ic_key_1) &&
               (ic_key_2 == rhs.ic_key_2);
    }
    bool operator!=(const Min3Key &rhs) const {
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
    Min3Parameters params_;
    size_t         serialized_size_;
};

class Min3KeyGenerator {
public:
    Min3KeyGenerator() = delete;
    explicit Min3KeyGenerator(
        const Min3Parameters       &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    void OfflineSetUp(const uint64_t num_eval, const std::string &file_path) const;

    std::pair<Min3Key, Min3Key> GenerateKeys() const;

private:
    Min3Parameters                params_;
    IntegerComparisonKeyGenerator gen_;
    sharing::AdditiveSharing2P   &ss_;
};

class Min3Evaluator {
public:
    Min3Evaluator() = delete;

    explicit Min3Evaluator(
        const Min3Parameters       &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path);

    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const Min3Key &key, const std::array<uint64_t, 3> &inputs) const;

private:
    Min3Parameters              params_;
    IntegerComparisonEvaluator  eval_;
    sharing::AdditiveSharing2P &ss_;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_MIN3_H_
