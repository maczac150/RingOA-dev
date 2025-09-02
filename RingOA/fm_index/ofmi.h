#ifndef FM_INDEX_OFMI_H_
#define FM_INDEX_OFMI_H_

#include "RingOA/protocol/zero_test.h"
#include "RingOA/wm/owm.h"

namespace ringoa {
namespace fm_index {

/**
 * @brief A class to hold parameters for the OFMI.
 */
class OFMIParameters {
public:
    /**
     * @brief Default constructor for OFMIParameters is deleted.
     */
    OFMIParameters() = delete;

    /**
     * @brief Parameterized constructor for OFMIParameters.
     * @param database_bitsize The database size for the OFMIParameters.
     * @param query_size The query size for the OFMIParameters.
     * @param sigma The alphabet size for the OFMIParameters.
     * @param mode The output type for the OFMIParameters.
     */
    OFMIParameters(const uint64_t database_bitsize,
                   const uint64_t query_size,
                   const uint64_t sigma = 3)
        : query_size_(query_size),
          owm_params_(database_bitsize, sigma),
          zt_params_(database_bitsize, database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the OFMIParameters.
     * @param database_bitsize The database size for the OFMIParameters.
     * @param query_size The query size for the OFMIParameters.
     * @param sigma The alphabet size for the OFMIParameters.
     * @param mode The output type for the OFMIParameters.
     */

    void ReconfigureParameters(const uint64_t database_bitsize,
                               const uint64_t query_size,
                               const uint64_t sigma = 3) {
        query_size_ = query_size;
        owm_params_.ReconfigureParameters(database_bitsize, sigma);
        zt_params_.ReconfigureParameters(database_bitsize, database_bitsize);
    }

    /**
     * @brief Get the database bit size for the OFMIParameters.
     * @return uint64_t The database size for the OFMIParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return owm_params_.GetDatabaseBitSize();
    }

    /**
     * @brief Get the database size for the OFMIParameters.
     * @return uint64_t The database size for the OFMIParameters.
     */
    uint64_t GetDatabaseSize() const {
        return owm_params_.GetDatabaseSize();
    }

    /**
     * @brief Get the query size for the OFMIParameters.
     * @return uint64_t The query size for the OFMIParameters.
     */
    uint64_t GetQuerySize() const {
        return query_size_;
    }

    /**
     * @brief Get the alphabet size for the OFMIParameters.
     * @return uint64_t The alphabet size for the OFMIParameters.
     */
    uint64_t GetSigma() const {
        return owm_params_.GetSigma();
    }

    /**
     * @brief Get the OWMParameters for the OFMIParameters.
     * @return const wm::OWMParameters& The OWMParameters for the OFMIParameters.
     */
    const wm::OWMParameters GetOWMParameters() const {
        return owm_params_;
    }

    /**
     * @brief Get the ZeroTestParameters for the OFMIParameters.
     * @return const ZeroTestParameters& The ZeroTestParameters for the OFMIParameters.
     */
    const proto::ZeroTestParameters GetZeroTestParameters() const {
        return zt_params_;
    }

    /**
     * @brief Get the parameters info for the OFMIParameters.
     * @return std::string The parameters info for the OFMIParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "Query size: " << query_size_ << ", " << owm_params_.GetParametersInfo() << ", " << zt_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the parameters info for the OFMIParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                  query_size_; /**< The query size for the OFMIParameters. */
    wm::OWMParameters         owm_params_; /**< The OWMParameters for the OFMIParameters. */
    proto::ZeroTestParameters zt_params_;  /**< The ZeroTestParameters for the OFMIParameters. */
};

/**
 * @brief A struct to hold the keys for the OFMI.
 */
struct OFMIKey {
    uint64_t                        num_wm_keys;
    uint64_t                        num_zt_keys;
    std::vector<wm::OWMKey>         wm_f_keys;
    std::vector<wm::OWMKey>         wm_g_keys;
    std::vector<proto::ZeroTestKey> zt_keys;

    /**
     * @brief Default constructor for OFMIKey is deleted.
     */
    OFMIKey() = delete;

    /**
     * @brief Parameterized constructor for OFMIKey.
     * @param id The ID for the OFMIKey.
     * @param params The OFMIParameters for the OFMIKey.
     */
    OFMIKey(const uint64_t id, const OFMIParameters &params);

    /**
     * @brief Default destructor for OFMIKey.
     */
    ~OFMIKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of OFMIKey.
     */
    OFMIKey(const OFMIKey &other)            = delete;
    OFMIKey &operator=(const OFMIKey &other) = delete;

    /**
     * @brief Move constructor for OFMIKey.
     */
    OFMIKey(OFMIKey &&) noexcept            = default;
    OFMIKey &operator=(OFMIKey &&) noexcept = default;

    /**
     * @brief Compare two OFMIKey for equality.
     * @param rhs The OFMIKey to compare with.
     * @return bool True if the OFMIKey are equal, false otherwise.
     */
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

    /**
     * @brief Calculate the size of the serialized OFMIKey.
     * @param buffer The buffer to store the serialized OFMIKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the OFMIKey.
     * @param buffer The buffer to store the serialized OFMIKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the OFMIKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    OFMIParameters params_; /**< The OFMIParameters for the OFMIKey. */
};

/**
 * @brief A class to generate keys for the OFMI.
 */
class OFMIKeyGenerator {
public:
    /**
     * @brief Default constructor for OFMIKeyGenerator is deleted.
     */
    OFMIKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for OFMIKeyGenerator.
     * @param params OWMParameters for the OFMIKeyGenerator.
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    OFMIKeyGenerator(
        const OFMIParameters         &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    void OfflineSetUp(const std::string &file_path);

    /**
     * @brief Generate shares for the database.
     * @param fm The FMIndex used to generate the shares.
     * @return std::array<std::vector<sharing::RepShareVec>, 3> The generated shares for the database.
     */
    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const wm::FMIndex &fm) const;

    /**
     * @brief Generate shares for the query.
     * @param fm The FMIndex used to generate the shares.
     * @param query The query to generate shares for.
     * @return std::array<sharing::RepShareVec, 3> The generated shares for the query.
     */
    std::array<sharing::RepShareMat64, 3> GenerateQueryU64Share(const wm::FMIndex &fm, std::string &query) const;

    /**
     * @brief Generate keys for the OFMI.
     * @return std::array<OFMIKey, 3> Array of generated OFMIKeys.
     */
    std::array<OFMIKey, 3> GenerateKeys() const;

private:
    OFMIParameters                params_; /**< OFMIParameters for the OFMIKeyGenerator. */
    wm::OWMKeyGenerator           wm_gen_; /**< OWMKeyGenerator for the OFMIKeyGenerator. */
    proto::ZeroTestKeyGenerator   zt_gen_; /**< ZeroTestKeyGenerator for the OFMIKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the OFMIEvaluator. */
};

/**
 * @brief A class to evaluate keys for the OFMI.
 */
class OFMIEvaluator {
public:
    /**
     * @brief Default constructor for OFMIEvaluator is deleted.
     */
    OFMIEvaluator() = delete;

    /**
     * @brief Parameterized constructor for OFMIEvaluator.
     * @param params OFMIParameters for the OFMIEvaluator.
     * @param rss Replicated sharing for 3-party for the OFMIEvaluator.
     * @param ass_prev Additive sharing for 2-party for the OFMIEvaluator.
     * @param ass_next Additive sharing for 2-party for the OFMIEvaluator.
     */
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
    OFMIParameters                params_;   /**< OFMIParameters for the OFMIEvaluator. */
    wm::OWMEvaluator              wm_eval_;  /**< OWMEvaluator for the OFMIEvaluator. */
    proto::ZeroTestEvaluator      zt_eval_;  /**< ZeroTestEvaluator for the OFMIEvaluator. */
    sharing::ReplicatedSharing3P &rss_;      /**< Replicated sharing for 3-party for the OFMIEvaluator. */
    sharing::AdditiveSharing2P   &ass_prev_; /**< Additive sharing for 2-party for the previous party. */
    sharing::AdditiveSharing2P   &ass_next_; /**< Additive sharing for 2-party for the next party. */
};

}    // namespace fm_index
}    // namespace ringoa

#endif    // FM_INDEX_OFMI_H_
