#ifndef WM_OWM_FSC_H_
#define WM_OWM_FSC_H_

#include "RingOA/protocol/ringoa_fsc.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class FMIndex;

class OWMFscParameters {
public:
    OWMFscParameters() = delete;
    OWMFscParameters(const uint64_t database_bitsize,
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

    const proto::RingOaFscParameters GetOaParameters() const {
        return oa_params_;
    }
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", RingOA params: " << oa_params_.GetParametersInfo();
        return oss.str();
    }
    void PrintParameters() const;

private:
    uint64_t                   database_bitsize_;
    uint64_t                   database_size_;
    uint64_t                   sigma_;
    proto::RingOaFscParameters oa_params_;
};

struct OWMFscKey {
    uint64_t                         num_oa_keys;
    std::vector<proto::RingOaFscKey> oa_keys;

    OWMFscKey() = delete;
    OWMFscKey(const uint64_t id, const OWMFscParameters &params);
    ~OWMFscKey() = default;

    OWMFscKey(const OWMFscKey &)                = delete;
    OWMFscKey &operator=(const OWMFscKey &)     = delete;
    OWMFscKey(OWMFscKey &&) noexcept            = default;
    OWMFscKey &operator=(OWMFscKey &&) noexcept = default;

    bool operator==(const OWMFscKey &rhs) const {
        return (num_oa_keys == rhs.num_oa_keys) && (oa_keys == rhs.oa_keys);
    }
    bool operator!=(const OWMFscKey &rhs) const {
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
    OWMFscParameters params_;
    size_t           serialized_size_;
};

class OWMFscKeyGenerator {
public:
    OWMFscKeyGenerator() = delete;
    OWMFscKeyGenerator(
        const OWMFscParameters       &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    const proto::RingOaFscKeyGenerator &GetRingOaFscKeyGenerator() const {
        return oa_gen_;
    }

    void GenerateDatabaseU64Share(const FMIndex                         &fm,
                                  std::array<sharing::RepShareMat64, 3> &db_sh,
                                  std::array<sharing::RepShareVec64, 3> &aux_sh,
                                  std::array<bool, 3>                   &v_sign) const;

    std::array<OWMFscKey, 3> GenerateKeys(std::array<bool, 3> &v_sign) const;

private:
    OWMFscParameters              params_;
    proto::RingOaFscKeyGenerator  oa_gen_;
    sharing::ReplicatedSharing3P &rss_;
};

class OWMFscEvaluator {
public:
    OWMFscEvaluator() = delete;
    OWMFscEvaluator(const OWMFscParameters       &params,
                    sharing::ReplicatedSharing3P &rss,
                    sharing::AdditiveSharing2P   &ass_prev,
                    sharing::AdditiveSharing2P   &ass_next);

    const proto::RingOaFscEvaluator &GetRingOaFscEvaluator() const {
        return oa_eval_;
    }

    void EvaluateRankCF(Channels                      &chls,
                        const OWMFscKey               &key,
                        std::vector<block>            &uv_prev,
                        std::vector<block>            &uv_next,
                        const sharing::RepShareMat64  &wm_tables,
                        const sharing::RepShareView64 &aux_sh,
                        const sharing::RepShareView64 &char_sh,
                        sharing::RepShare64           &position_sh,
                        sharing::RepShare64           &result) const;

    void EvaluateRankCF_Parallel(Channels                      &chls,
                                 const OWMFscKey               &key1,
                                 const OWMFscKey               &key2,
                                 std::vector<block>            &uv_prev,
                                 std::vector<block>            &uv_next,
                                 const sharing::RepShareMat64  &wm_tables,
                                 const sharing::RepShareView64 &aux_sh,
                                 const sharing::RepShareView64 &char_sh,
                                 sharing::RepShareVec64        &position_sh,
                                 sharing::RepShareVec64        &result) const;

private:
    OWMFscParameters              params_;
    proto::RingOaFscEvaluator     oa_eval_;
    sharing::ReplicatedSharing3P &rss_;
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_OWM_FSC_H_
