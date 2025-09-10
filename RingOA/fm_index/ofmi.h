#ifndef FM_INDEX_OFMI_H_
#define FM_INDEX_OFMI_H_

#include "RingOA/protocol/zero_test.h"
#include "RingOA/wm/owm.h"

namespace ringoa {
namespace fm_index {

class OFMIParameters {
public:
    OFMIParameters() = delete;
    OFMIParameters(const uint64_t database_bitsize,
                   const uint64_t query_size,
                   const uint64_t sigma = 3)
        : query_size_(query_size),
          owm_params_(database_bitsize, sigma),
          zt_params_(database_bitsize, database_bitsize) {
    }

    void ReconfigureParameters(const uint64_t database_bitsize,
                               const uint64_t query_size,
                               const uint64_t sigma = 3) {
        query_size_ = query_size;
        owm_params_.ReconfigureParameters(database_bitsize, sigma);
        zt_params_.ReconfigureParameters(database_bitsize, database_bitsize);
    }

    uint64_t GetDatabaseBitSize() const {
        return owm_params_.GetDatabaseBitSize();
    }
    uint64_t GetDatabaseSize() const {
        return owm_params_.GetDatabaseSize();
    }
    uint64_t GetQuerySize() const {
        return query_size_;
    }
    uint64_t GetSigma() const {
        return owm_params_.GetSigma();
    }

    const wm::OWMParameters GetOWMParameters() const {
        return owm_params_;
    }
    const proto::ZeroTestParameters GetZeroTestParameters() const {
        return zt_params_;
    }
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "Query size: " << query_size_
            << ", " << owm_params_.GetParametersInfo()
            << ", " << zt_params_.GetParametersInfo();
        return oss.str();
    }
    void PrintParameters() const;

private:
    uint64_t                  query_size_;
    wm::OWMParameters         owm_params_;
    proto::ZeroTestParameters zt_params_;
};

struct OFMIKey {
    uint64_t                        num_wm_keys;
    uint64_t                        num_zt_keys;
    std::vector<wm::OWMKey>         wm_f_keys;
    std::vector<wm::OWMKey>         wm_g_keys;
    std::vector<proto::ZeroTestKey> zt_keys;

    OFMIKey() = delete;
    OFMIKey(const uint64_t id, const OFMIParameters &params);
    ~OFMIKey() = default;

    OFMIKey(const OFMIKey &other)            = delete;
    OFMIKey &operator=(const OFMIKey &other) = delete;
    OFMIKey(OFMIKey &&) noexcept             = default;
    OFMIKey &operator=(OFMIKey &&) noexcept  = default;

    bool operator==(const OFMIKey &rhs) const {
        return (num_wm_keys == rhs.num_wm_keys) &&
               (num_zt_keys == rhs.num_zt_keys) &&
               (wm_f_keys == rhs.wm_f_keys) &&
               (wm_g_keys == rhs.wm_g_keys) &&
               (zt_keys == rhs.zt_keys);
    }
    bool operator!=(const OFMIKey &rhs) const {
        return !(*this == rhs);
    }

    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
    void PrintKey(const bool detailed = false) const;

private:
    OFMIParameters params_;
};

class OFMIKeyGenerator {
public:
    OFMIKeyGenerator() = delete;
    OFMIKeyGenerator(
        const OFMIParameters         &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    void OfflineSetUp(const std::string &file_path);

    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const wm::FMIndex &fm) const;
    std::array<sharing::RepShareMat64, 3> GenerateQueryU64Share(const wm::FMIndex &fm, std::string &query) const;

    std::array<OFMIKey, 3> GenerateKeys() const;

private:
    OFMIParameters                params_;
    wm::OWMKeyGenerator           wm_gen_;
    proto::ZeroTestKeyGenerator   zt_gen_;
    sharing::ReplicatedSharing3P &rss_;
};

class OFMIEvaluator {
public:
    OFMIEvaluator() = delete;
    OFMIEvaluator(const OFMIParameters         &params,
                  sharing::ReplicatedSharing3P &rss,
                  sharing::AdditiveSharing2P   &ass_prev,
                  sharing::AdditiveSharing2P   &ass_next);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path);

    void EvaluateLPM(Channels                     &chls,
                     const OFMIKey                &key,
                     std::vector<block>           &uv_prev,
                     std::vector<block>           &uv_next,
                     const sharing::RepShareMat64 &wm_tables,
                     const sharing::RepShareMat64 &query,
                     sharing::RepShareVec64       &result) const;

    void EvaluateLPM_Parallel(Channels                     &chls,
                              const OFMIKey                &key,
                              std::vector<block>           &uv_prev,
                              std::vector<block>           &uv_next,
                              const sharing::RepShareMat64 &wm_tables,
                              const sharing::RepShareMat64 &query,
                              sharing::RepShareVec64       &result) const;

private:
    OFMIParameters                params_;
    wm::OWMEvaluator              wm_eval_;
    proto::ZeroTestEvaluator      zt_eval_;
    sharing::ReplicatedSharing3P &rss_;
    sharing::AdditiveSharing2P   &ass_prev_;
    sharing::AdditiveSharing2P   &ass_next_;
};

}    // namespace fm_index
}    // namespace ringoa

#endif    // FM_INDEX_OFMI_H_
