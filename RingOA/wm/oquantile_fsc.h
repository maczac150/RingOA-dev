#ifndef WM_OQUANTILE_FSC_H_
#define WM_OQUANTILE_FSC_H_

#include "RingOA/protocol/integer_comparison.h"
#include "RingOA/protocol/ringoa_fsc.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class WaveletMatrix;

class OQuantileFscParameters {
public:
    OQuantileFscParameters() = delete;
    OQuantileFscParameters(const uint64_t database_bitsize,
                           const uint64_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          share_size_(database_bitsize + 1),
          sigma_(sigma),
          oa_params_(database_bitsize, share_size_),
          ic_params_(share_size_, share_size_) {
    }

    void ReconfigureParameters(const uint64_t database_bitsize, const uint64_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        share_size_       = database_bitsize + 1;
        sigma_            = sigma;
        oa_params_.ReconfigureParameters(database_bitsize, share_size_);
        ic_params_.ReconfigureParameters(share_size_, share_size_);
    }

    uint64_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }
    uint64_t GetDatabaseSize() const {
        return database_size_;
    }
    uint64_t GetShareSize() const {
        return share_size_;
    }
    uint64_t GetSigma() const {
        return sigma_;
    }

    const proto::RingOaFscParameters GetOaParameters() const {
        return oa_params_;
    }
    const proto::IntegerComparisonParameters GetIcParameters() const {
        return ic_params_;
    }
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_
            << ", Share size: " << share_size_
            << ", Sigma: " << sigma_
            << ", RingOA params: "
            << oa_params_.GetParametersInfo()
            << ", IC params: " << ic_params_.GetParametersInfo();
        return oss.str();
    }
    void PrintParameters() const;

private:
    uint64_t                           database_bitsize_;
    uint64_t                           database_size_;
    uint64_t                           share_size_;
    uint64_t                           sigma_;
    proto::RingOaFscParameters         oa_params_;
    proto::IntegerComparisonParameters ic_params_;
};

struct
    OQuantileFscKey {
    uint64_t                                 num_oa_keys;
    uint64_t                                 num_ic_keys;
    std::vector<proto::RingOaFscKey>         oa_keys;
    std::vector<proto::IntegerComparisonKey> ic_keys;

    OQuantileFscKey() = delete;
    OQuantileFscKey(const uint64_t id, const OQuantileFscParameters &params);
    ~OQuantileFscKey() = default;

    OQuantileFscKey(const OQuantileFscKey &)                = delete;
    OQuantileFscKey &operator=(const OQuantileFscKey &)     = delete;
    OQuantileFscKey(OQuantileFscKey &&) noexcept            = default;
    OQuantileFscKey &operator=(OQuantileFscKey &&) noexcept = default;

    bool operator==(const OQuantileFscKey &rhs) const {
        return (num_oa_keys == rhs.num_oa_keys) &&
               (num_ic_keys == rhs.num_ic_keys) &&
               (oa_keys == rhs.oa_keys) &&
               (ic_keys == rhs.ic_keys);
    }
    bool operator!=(const OQuantileFscKey &rhs) const {
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
    OQuantileFscParameters params_;
    size_t                 serialized_size_;
};

class OQuantileFscKeyGenerator {
public:
    OQuantileFscKeyGenerator() = delete;
    OQuantileFscKeyGenerator(
        const OQuantileFscParameters &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    const proto::RingOaFscKeyGenerator &GetRingOaFscKeyGenerator() const {
        return oa_gen_;
    }

    void GenerateDatabaseU64Share(const WaveletMatrix                   &wm,
                                  std::array<sharing::RepShareMat64, 3> &db_sh,
                                  std::array<sharing::RepShareVec64, 3> &aux_sh,
                                  std::array<bool, 3>                   &v_sign) const;

    std::array<OQuantileFscKey, 3> GenerateKeys(std::array<bool, 3> &v_sign) const;

private:
    OQuantileFscParameters               params_;
    proto::RingOaFscKeyGenerator         oa_gen_;
    proto::IntegerComparisonKeyGenerator ic_gen_;
    sharing::ReplicatedSharing3P        &rss_;
};

class OQuantileFscEvaluator {
public:
    OQuantileFscEvaluator() = delete;
    OQuantileFscEvaluator(const OQuantileFscParameters &params,
                          sharing::ReplicatedSharing3P &rss,
                          sharing::AdditiveSharing2P   &ass_prev,
                          sharing::AdditiveSharing2P   &ass_next);

    void EvaluateQuantile(Channels                      &chls,
                          const OQuantileFscKey         &key,
                          std::vector<block>            &uv_prev,
                          std::vector<block>            &uv_next,
                          const sharing::RepShareMat64  &wm_tables,
                          const sharing::RepShareView64 &aux_sh,
                          sharing::RepShare64           &left_sh,
                          sharing::RepShare64           &right_sh,
                          sharing::RepShare64           &k_sh,
                          sharing::RepShare64           &result) const;

    void EvaluateQuantile_Parallel(Channels                      &chls,
                                   const OQuantileFscKey         &key,
                                   std::vector<block>            &uv_prev,
                                   std::vector<block>            &uv_next,
                                   const sharing::RepShareMat64  &wm_tables,
                                   const sharing::RepShareView64 &aux_sh,
                                   sharing::RepShare64           &left_sh,
                                   sharing::RepShare64           &right_sh,
                                   sharing::RepShare64           &k_sh,
                                   sharing::RepShare64           &result) const;

private:
    OQuantileFscParameters            params_;
    proto::RingOaFscEvaluator         oa_eval_;
    proto::IntegerComparisonEvaluator ic_eval_;
    sharing::ReplicatedSharing3P     &rss_;
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_OQUANTILE_FSC_H_
