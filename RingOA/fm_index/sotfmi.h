#ifndef FM_INDEX_SOTFMI_H_
#define FM_INDEX_SOTFMI_H_

#include "RingOA/protocol/zero_test.h"
#include "RingOA/wm/sotwm.h"

namespace ringoa {
namespace fm_index {

class SotFMIParameters {
public:
    SotFMIParameters() = delete;
    SotFMIParameters(const uint64_t      database_bitsize,
                     const uint64_t      query_size,
                     const uint64_t      sigma = 3,
                     const fss::EvalType type  = fss::kOptimizedEvalType)
        : query_size_(query_size),
          sotwm_params_(database_bitsize, sigma, type),
          zt_params_(database_bitsize, database_bitsize) {
    }

    void ReconfigureParameters(const uint64_t      database_bitsize,
                               const uint64_t      query_size,
                               const uint64_t      sigma = 3,
                               const fss::EvalType type  = fss::kOptimizedEvalType) {
        query_size_ = query_size;
        sotwm_params_.ReconfigureParameters(database_bitsize, sigma, type);
        zt_params_.ReconfigureParameters(database_bitsize, database_bitsize);
    }

    uint64_t GetDatabaseBitSize() const {
        return sotwm_params_.GetDatabaseBitSize();
    }
    uint64_t GetDatabaseSize() const {
        return sotwm_params_.GetDatabaseSize();
    }
    uint64_t GetQuerySize() const {
        return query_size_;
    }
    uint64_t GetSigma() const {
        return sotwm_params_.GetSigma();
    }

    const wm::SotWMParameters GetSotWMParameters() const {
        return sotwm_params_;
    }
    const proto::ZeroTestParameters GetZeroTestParameters() const {
        return zt_params_;
    }
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "Query size: " << query_size_ << ", " << sotwm_params_.GetParametersInfo() << ", " << zt_params_.GetParametersInfo();
        return oss.str();
    }
    void PrintParameters() const;

private:
    uint64_t                  query_size_;
    wm::SotWMParameters       sotwm_params_;
    proto::ZeroTestParameters zt_params_;
};

struct SotFMIKey {
    uint64_t                        num_wm_keys;
    uint64_t                        num_zt_keys;
    std::vector<wm::SotWMKey>       wm_f_keys;
    std::vector<wm::SotWMKey>       wm_g_keys;
    std::vector<proto::ZeroTestKey> zt_keys;

    SotFMIKey() = delete;
    SotFMIKey(const uint64_t id, const SotFMIParameters &params);
    ~SotFMIKey() = default;

    SotFMIKey(const SotFMIKey &other)            = delete;
    SotFMIKey &operator=(const SotFMIKey &other) = delete;
    SotFMIKey(SotFMIKey &&) noexcept             = default;
    SotFMIKey &operator=(SotFMIKey &&) noexcept  = default;

    bool operator==(const SotFMIKey &rhs) const {
        return (num_wm_keys == rhs.num_wm_keys) &&
               (num_zt_keys == rhs.num_zt_keys) &&
               (wm_f_keys == rhs.wm_f_keys) &&
               (wm_g_keys == rhs.wm_g_keys) &&
               (zt_keys == rhs.zt_keys);
    }
    bool operator!=(const SotFMIKey &rhs) const {
        return !(*this == rhs);
    }

    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);

    void PrintKey(const bool detailed = false) const;

private:
    SotFMIParameters params_;
};

class SotFMIKeyGenerator {
public:
    SotFMIKeyGenerator() = delete;
    SotFMIKeyGenerator(
        const SotFMIParameters       &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const wm::FMIndex &fm) const;
    std::array<sharing::RepShareMat64, 3> GenerateQueryU64Share(const wm::FMIndex &fm, std::string &query) const;

    std::array<SotFMIKey, 3> GenerateKeys() const;

private:
    SotFMIParameters              params_;
    wm::SotWMKeyGenerator         wm_gen_;
    proto::ZeroTestKeyGenerator   zt_gen_;
    sharing::ReplicatedSharing3P &rss_;
};

class SotFMIEvaluator {
public:
    SotFMIEvaluator() = delete;
    SotFMIEvaluator(const SotFMIParameters       &params,
                    sharing::ReplicatedSharing3P &rss,
                    sharing::AdditiveSharing2P   &ass);

    void EvaluateLPM(Channels                     &chls,
                     const SotFMIKey              &key,
                     std::vector<uint64_t>        &uv_prev,
                     std::vector<uint64_t>        &uv_next,
                     const sharing::RepShareMat64 &wm_tables,
                     const sharing::RepShareMat64 &query,
                     sharing::RepShareVec64       &result) const;

    void EvaluateLPM_Parallel(Channels                     &chls,
                              const SotFMIKey              &key,
                              std::vector<uint64_t>        &uv_prev,
                              std::vector<uint64_t>        &uv_next,
                              const sharing::RepShareMat64 &wm_tables,
                              const sharing::RepShareMat64 &query,
                              sharing::RepShareVec64       &result) const;

private:
    SotFMIParameters              params_;
    wm::SotWMEvaluator            wm_eval_;
    proto::ZeroTestEvaluator      zt_eval_;
    sharing::ReplicatedSharing3P &rss_;
    sharing::AdditiveSharing2P   &ass_;
};

}    // namespace fm_index
}    // namespace ringoa

#endif    // FM_INDEX_SOTFMI_H_
