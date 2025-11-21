#ifndef WM_OQUANTILE_H_
#define WM_OQUANTILE_H_

#include "RingOA/protocol/integer_comparison.h"
#include "RingOA/protocol/ringoa.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class WaveletMatrix;

class OQuantileParameters {
public:
    OQuantileParameters() = delete;
    OQuantileParameters(const uint64_t database_bitsize,
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

    const proto::RingOaParameters GetOaParameters() const {
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
    proto::RingOaParameters            oa_params_;
    proto::IntegerComparisonParameters ic_params_;
};

struct 
OQuantileKey {
    uint64_t                                 num_oa_keys;
    uint64_t                                 num_ic_keys;
    std::vector<proto::RingOaKey>            oa_keys;
    std::vector<proto::IntegerComparisonKey> ic_keys;

    OQuantileKey() = delete;
    OQuantileKey(const uint64_t id, const OQuantileParameters &params);
    ~OQuantileKey() = default;

    OQuantileKey(const OQuantileKey &)                = delete;
    OQuantileKey &operator=(const OQuantileKey &)     = delete;
    OQuantileKey(OQuantileKey &&) noexcept            = default;
    OQuantileKey &operator=(OQuantileKey &&) noexcept = default;

    bool operator==(const OQuantileKey &rhs) const {
        return (num_oa_keys == rhs.num_oa_keys) &&
               (num_ic_keys == rhs.num_ic_keys) &&
               (oa_keys == rhs.oa_keys) &&
               (ic_keys == rhs.ic_keys);
    }
    bool operator!=(const OQuantileKey &rhs) const {
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
    OQuantileParameters params_;
    size_t              serialized_size_;
};

class OQuantileKeyGenerator {
public:
    OQuantileKeyGenerator() = delete;
    OQuantileKeyGenerator(
        const OQuantileParameters    &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    void OfflineSetUp(const std::string &file_path);

    const proto::RingOaKeyGenerator &GetRingOaKeyGenerator() const {
        return oa_gen_;
    }

    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const WaveletMatrix &wm) const;

    std::array<OQuantileKey, 3> GenerateKeys() const;

private:
    OQuantileParameters                  params_;
    proto::RingOaKeyGenerator            oa_gen_;
    proto::IntegerComparisonKeyGenerator ic_gen_;
    sharing::ReplicatedSharing3P        &rss_;
};

class OQuantileEvaluator {
public:
    OQuantileEvaluator() = delete;
    OQuantileEvaluator(const OQuantileParameters    &params,
                       sharing::ReplicatedSharing3P &rss,
                       sharing::AdditiveSharing2P   &ass_prev,
                       sharing::AdditiveSharing2P   &ass_next);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path);

    void EvaluateQuantile(Channels                     &chls,
                          const OQuantileKey           &key,
                          std::vector<block>           &uv_prev,
                          std::vector<block>           &uv_next,
                          const sharing::RepShareMat64 &wm_tables,
                          sharing::RepShare64          &left_sh,
                          sharing::RepShare64          &right_sh,
                          sharing::RepShare64          &k_sh,
                          sharing::RepShare64          &result) const;

    void EvaluateQuantile_Parallel(Channels                     &chls,
                                   const OQuantileKey           &key,
                                   std::vector<block>           &uv_prev,
                                   std::vector<block>           &uv_next,
                                   const sharing::RepShareMat64 &wm_tables,
                                   sharing::RepShare64          &left_sh,
                                   sharing::RepShare64          &right_sh,
                                   sharing::RepShare64          &k_sh,
                                   sharing::RepShare64          &result) const;

private:
    OQuantileParameters               params_;
    proto::RingOaEvaluator            oa_eval_;
    proto::IntegerComparisonEvaluator ic_eval_;
    sharing::ReplicatedSharing3P     &rss_;
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_OQUANTILE_H_
