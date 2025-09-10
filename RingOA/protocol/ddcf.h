#ifndef PROTOCOL_DDCF_H_
#define PROTOCOL_DDCF_H_

#include "RingOA/fss/dcf_eval.h"
#include "RingOA/fss/dcf_gen.h"
#include "RingOA/fss/dcf_key.h"

namespace ringoa {
namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace proto {

class DdcfParameters {
public:
    DdcfParameters() = delete;
    explicit DdcfParameters(const uint64_t n, const uint64_t e)
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

    const fss::dcf::DcfParameters GetParameters() const {
        return params_;
    }

    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }

    void PrintParameters() const;

private:
    fss::dcf::DcfParameters params_; /**< DCF parameters for the DDCF. */
};

struct DdcfKey {
    fss::dcf::DcfKey dcf_key;
    uint64_t         mask;

    DdcfKey() = delete;
    explicit DdcfKey(const uint64_t id, const DdcfParameters &params);
    ~DdcfKey() = default;

    DdcfKey(const DdcfKey &)                = delete;
    DdcfKey &operator=(const DdcfKey &)     = delete;
    DdcfKey(DdcfKey &&) noexcept            = default;
    DdcfKey &operator=(DdcfKey &&) noexcept = default;

    bool operator==(const DdcfKey &rhs) const {
        return dcf_key == rhs.dcf_key &&
               mask == rhs.mask;
    }
    bool operator!=(const DdcfKey &rhs) const {
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
    DdcfParameters params_;
    size_t         serialized_size_;
};

class DdcfKeyGenerator {
public:
    DdcfKeyGenerator() = delete;
    explicit DdcfKeyGenerator(const DdcfParameters &params);

    std::pair<DdcfKey, DdcfKey> GenerateKeys(uint64_t alpha, uint64_t beta_1, uint64_t beta_2) const;

private:
    DdcfParameters            params_;
    fss::dcf::DcfKeyGenerator gen_;
};

class DdcfEvaluator {
public:
    DdcfEvaluator() = delete;
    explicit DdcfEvaluator(const DdcfParameters &params);

    uint64_t EvaluateAt(const DdcfKey &key, uint64_t x) const;

private:
    DdcfParameters         params_;
    fss::dcf::DcfEvaluator eval_;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_DDCF_H_
