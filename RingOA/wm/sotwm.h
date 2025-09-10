#ifndef WM_SOTWM_H_
#define WM_SOTWM_H_

#include "RingOA/protocol/shared_ot.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class FMIndex;

class SotWMParameters {
public:
    SotWMParameters() = delete;
    SotWMParameters(const uint64_t      database_bitsize,
                    const uint64_t      sigma = 3,
                    const fss::EvalType type  = fss::kOptimizedEvalType)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          sigma_(sigma),
          sot_params_(database_bitsize, type) {
    }

    void ReconfigureParameters(const uint64_t      database_bitsize,
                               const uint64_t      sigma = 3,
                               const fss::EvalType type  = fss::kOptimizedEvalType) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        sigma_            = sigma;
        sot_params_.ReconfigureParameters(database_bitsize, type);
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

    const proto::SharedOtParameters GetSotParameters() const {
        return sot_params_;
    }
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", SOT params: " << sot_params_.GetParametersInfo();
        return oss.str();
    }
    void PrintParameters() const;

private:
    uint64_t                  database_bitsize_;
    uint64_t                  database_size_;
    uint64_t                  sigma_;
    proto::SharedOtParameters sot_params_;
};

struct SotWMKey {
    uint64_t                        num_sot_keys;
    std::vector<proto::SharedOtKey> sot_keys;

    SotWMKey() = delete;
    SotWMKey(const uint64_t id, const SotWMParameters &params);
    ~SotWMKey() = default;

    SotWMKey(const SotWMKey &)                = delete;
    SotWMKey &operator=(const SotWMKey &)     = delete;
    SotWMKey(SotWMKey &&) noexcept            = default;
    SotWMKey &operator=(SotWMKey &&) noexcept = default;

    bool operator==(const SotWMKey &rhs) const {
        return (num_sot_keys == rhs.num_sot_keys) && (sot_keys == rhs.sot_keys);
    }
    bool operator!=(const SotWMKey &rhs) const {
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
    SotWMParameters params_;
    size_t          serialized_size_;
};

class SotWMKeyGenerator {
public:
    SotWMKeyGenerator() = delete;
    SotWMKeyGenerator(
        const SotWMParameters        &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    const proto::SharedOtKeyGenerator &GetSharedOtKeyGenerator() const {
        return sot_gen_;
    }

    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const FMIndex &fm) const;

    std::array<SotWMKey, 3> GenerateKeys() const;

private:
    SotWMParameters               params_;
    proto::SharedOtKeyGenerator   sot_gen_;
    sharing::ReplicatedSharing3P &rss_;
};

class SotWMEvaluator {
public:
    SotWMEvaluator() = delete;
    SotWMEvaluator(const SotWMParameters        &params,
                   sharing::ReplicatedSharing3P &rss);

    const proto::SharedOtEvaluator &GetSharedOtEvaluator() const {
        return sot_eval_;
    }

    void EvaluateRankCF(Channels                      &chls,
                        const SotWMKey                &key,
                        std::vector<uint64_t>         &uv_prev,
                        std::vector<uint64_t>         &uv_next,
                        const sharing::RepShareMat64  &wm_tables,
                        const sharing::RepShareView64 &char_sh,
                        sharing::RepShare64           &position_sh,
                        sharing::RepShare64           &result) const;

    void EvaluateRankCF_Parallel(Channels                      &chls,
                                 const SotWMKey                &key1,
                                 const SotWMKey                &key2,
                                 std::vector<uint64_t>         &uv_prev,
                                 std::vector<uint64_t>         &uv_next,
                                 const sharing::RepShareMat64  &wm_tables,
                                 const sharing::RepShareView64 &char_sh,
                                 sharing::RepShareVec64        &position_sh,
                                 sharing::RepShareVec64        &result) const;

private:
    SotWMParameters               params_;
    proto::SharedOtEvaluator      sot_eval_;
    sharing::ReplicatedSharing3P &rss_;
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_SOTWM_H_
