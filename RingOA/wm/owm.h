#ifndef WM_OWM_H_
#define WM_OWM_H_

#include "RingOA/protocol/ringoa.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class FMIndex;

class OWMParameters {
public:
    OWMParameters() = delete;
    OWMParameters(const uint64_t database_bitsize,
                  const uint64_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          sigma_(sigma),
          oa_params_(database_bitsize) {
    }

    void ReconfigureParameters(const uint64_t database_bitsize, const uint64_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        sigma_            = sigma;
        oa_params_.ReconfigureParameters(database_bitsize);
    }

    uint64_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }
    uint64_t GetDatabaseSize() const {
        return database_size_;
    }
    uint64_t GetSigma() const {
        return sigma_;
    }

    const proto::RingOaParameters GetOaParameters() const {
        return oa_params_;
    }
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", RingOA params: " << oa_params_.GetParametersInfo();
        return oss.str();
    }
    void PrintParameters() const;

private:
    uint64_t                database_bitsize_;
    uint64_t                database_size_;
    uint64_t                sigma_;
    proto::RingOaParameters oa_params_;
};

struct OWMKey {
    uint64_t                      num_oa_keys;
    std::vector<proto::RingOaKey> oa_keys;

    OWMKey() = delete;
    OWMKey(const uint64_t id, const OWMParameters &params);
    ~OWMKey() = default;

    OWMKey(const OWMKey &)                = delete;
    OWMKey &operator=(const OWMKey &)     = delete;
    OWMKey(OWMKey &&) noexcept            = default;
    OWMKey &operator=(OWMKey &&) noexcept = default;

    bool operator==(const OWMKey &rhs) const {
        return (num_oa_keys == rhs.num_oa_keys) && (oa_keys == rhs.oa_keys);
    }
    bool operator!=(const OWMKey &rhs) const {
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
    OWMParameters params_;
    size_t        serialized_size_;
};

class OWMKeyGenerator {
public:
    OWMKeyGenerator() = delete;
    OWMKeyGenerator(
        const OWMParameters          &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    const proto::RingOaKeyGenerator &GetRingOaKeyGenerator() const {
        return oa_gen_;
    }

    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const FMIndex &fm) const;

    std::array<OWMKey, 3> GenerateKeys() const;

private:
    OWMParameters                 params_;
    proto::RingOaKeyGenerator     oa_gen_;
    sharing::ReplicatedSharing3P &rss_;
};

class OWMEvaluator {
public:
    OWMEvaluator() = delete;
    OWMEvaluator(const OWMParameters          &params,
                 sharing::ReplicatedSharing3P &rss,
                 sharing::AdditiveSharing2P   &ass_prev,
                 sharing::AdditiveSharing2P   &ass_next);

    const proto::RingOaEvaluator &GetRingOaEvaluator() const {
        return oa_eval_;
    }

    void EvaluateRankCF(Channels                      &chls,
                        const OWMKey                  &key,
                        std::vector<block>            &uv_prev,
                        std::vector<block>            &uv_next,
                        const sharing::RepShareMat64  &wm_tables,
                        const sharing::RepShareView64 &char_sh,
                        sharing::RepShare64           &position_sh,
                        sharing::RepShare64           &result) const;

    void EvaluateRankCF_Parallel(Channels                      &chls,
                                 const OWMKey                  &key1,
                                 const OWMKey                  &key2,
                                 std::vector<block>            &uv_prev,
                                 std::vector<block>            &uv_next,
                                 const sharing::RepShareMat64  &wm_tables,
                                 const sharing::RepShareView64 &char_sh,
                                 sharing::RepShareVec64        &position_sh,
                                 sharing::RepShareVec64        &result) const;

private:
    OWMParameters                 params_;
    proto::RingOaEvaluator        oa_eval_;
    sharing::ReplicatedSharing3P &rss_;
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_OWM_H_
