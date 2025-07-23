#ifndef FM_INDEX_SOTFMI_H_
#define FM_INDEX_SOTFMI_H_

#include "RingOA/protocol/zero_test.h"
#include "RingOA/wm/sotwm.h"

namespace ringoa {
namespace fm_index {

/**
 * @brief A class to hold parameters for the SotFMI.
 */
class SotFMIParameters {
public:
    /**
     * @brief Default constructor for SotFMIParameters is deleted.
     */
    SotFMIParameters() = delete;

    /**
     * @brief Parameterized constructor for SotFMIParameters.
     * @param database_bitsize The database size for the SotFMIParameters.
     * @param query_size The query size for the SotFMIParameters.
     * @param sigma The alphabet size for the SotFMIParameters.
     * @param mode The output type for the SotFMIParameters.
     */
    SotFMIParameters(const uint64_t database_bitsize,
                     const uint64_t query_size,
                     const uint64_t sigma = 3)
        : query_size_(query_size),
          sotwm_params_(database_bitsize, sigma),
          zt_params_(database_bitsize, database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the SotFMIParameters.
     * @param database_bitsize The database size for the SotFMIParameters.
     * @param query_size The query size for the SotFMIParameters.
     * @param sigma The alphabet size for the SotFMIParameters.
     * @param mode The output type for the SotFMIParameters.
     */

    void ReconfigureParameters(const uint64_t database_bitsize,
                               const uint64_t query_size,
                               const uint64_t sigma = 3) {
        query_size_ = query_size;
        sotwm_params_.ReconfigureParameters(database_bitsize, sigma);
        zt_params_.ReconfigureParameters(database_bitsize, database_bitsize);
    }

    /**
     * @brief Get the database bit size for the SotFMIParameters.
     * @return uint64_t The database size for the SotFMIParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return sotwm_params_.GetDatabaseBitSize();
    }

    /**
     * @brief Get the database size for the SotFMIParameters.
     * @return uint64_t The database size for the SotFMIParameters.
     */
    uint64_t GetDatabaseSize() const {
        return sotwm_params_.GetDatabaseSize();
    }

    /**
     * @brief Get the query size for the SotFMIParameters.
     * @return uint64_t The query size for the SotFMIParameters.
     */
    uint64_t GetQuerySize() const {
        return query_size_;
    }

    /**
     * @brief Get the alphabet size for the SotFMIParameters.
     * @return uint64_t The alphabet size for the SotFMIParameters.
     */
    uint64_t GetSigma() const {
        return sotwm_params_.GetSigma();
    }

    /**
     * @brief Get the SotWMParameters for the SotFMIParameters.
     * @return const wm::SotWMParameters& The SotWMParameters for the SotFMIParameters.
     */
    const wm::SotWMParameters GetSotWMParameters() const {
        return sotwm_params_;
    }

    /**
     * @brief Get the ZeroTestParameters for the SotFMIParameters.
     * @return const ZeroTestParameters& The ZeroTestParameters for the SotFMIParameters.
     */
    const proto::ZeroTestParameters GetZeroTestParameters() const {
        return zt_params_;
    }

    /**
     * @brief Get the parameters info for the SotFMIParameters.
     * @return std::string The parameters info for the SotFMIParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "Query size: " << query_size_ << ", " << sotwm_params_.GetParametersInfo() << ", " << zt_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the parameters info for the SotFMIParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                  query_size_;   /**< The query size for the SotFMIParameters. */
    wm::SotWMParameters       sotwm_params_; /**< The SotWMParameters for the SotFMIParameters. */
    proto::ZeroTestParameters zt_params_;    /**< The ZeroTestParameters for the SotFMIParameters. */
};

/**
 * @brief A struct to hold the keys for the SotFMI.
 */
struct SotFMIKey {
    uint64_t                        num_wm_keys;
    uint64_t                        num_zt_keys;
    std::vector<wm::SotWMKey>       wm_f_keys;
    std::vector<wm::SotWMKey>       wm_g_keys;
    std::vector<proto::ZeroTestKey> zt_keys;

    /**
     * @brief Default constructor for SotFMIKey is deleted.
     */
    SotFMIKey() = delete;

    /**
     * @brief Parameterized constructor for SotFMIKey.
     * @param id The ID for the SotFMIKey.
     * @param params The SotFMIParameters for the SotFMIKey.
     */
    SotFMIKey(const uint64_t id, const SotFMIParameters &params);

    /**
     * @brief Default destructor for SotFMIKey.
     */
    ~SotFMIKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of SotFMIKey.
     */
    SotFMIKey(const SotFMIKey &other)            = delete;
    SotFMIKey &operator=(const SotFMIKey &other) = delete;

    /**
     * @brief Move constructor for SotFMIKey.
     */
    SotFMIKey(SotFMIKey &&) noexcept            = default;
    SotFMIKey &operator=(SotFMIKey &&) noexcept = default;

    /**
     * @brief Compare two SotFMIKey for equality.
     * @param rhs The SotFMIKey to compare with.
     * @return bool True if the SotFMIKey are equal, false otherwise.
     */
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

    /**
     * @brief Calculate the size of the serialized SotFMIKey.
     * @param buffer The buffer to store the serialized SotFMIKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the SotFMIKey.
     * @param buffer The buffer to store the serialized SotFMIKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the SotFMIKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    SotFMIParameters params_; /**< The SotFMIParameters for the SotFMIKey. */
};

/**
 * @brief A class to generate keys for the SotFMI.
 */
class SotFMIKeyGenerator {
public:
    /**
     * @brief Default constructor for SotFMIKeyGenerator is deleted.
     */
    SotFMIKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for SotFMIKeyGenerator.
     * @param params SotWMParameters for the SotFMIKeyGenerator.
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    SotFMIKeyGenerator(
        const SotFMIParameters       &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

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
     * @brief Generate keys for the SotFMI.
     * @return std::array<SotFMIKey, 3> Array of generated SotFMIKeys.
     */
    std::array<SotFMIKey, 3> GenerateKeys() const;

private:
    SotFMIParameters              params_; /**< SotFMIParameters for the SotFMIKeyGenerator. */
    wm::SotWMKeyGenerator         wm_gen_; /**< SotWMKeyGenerator for the SotFMIKeyGenerator. */
    proto::ZeroTestKeyGenerator   zt_gen_; /**< ZeroTestKeyGenerator for the SotFMIKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the SotFMIEvaluator. */
};

/**
 * @brief A class to evaluate keys for the SotFMI.
 */
class SotFMIEvaluator {
public:
    /**
     * @brief Default constructor for SotFMIEvaluator is deleted.
     */
    SotFMIEvaluator() = delete;

    /**
     * @brief Parameterized constructor for SotFMIEvaluator.
     * @param params SotFMIParameters for the SotFMIEvaluator.
     * @param rss Replicated sharing for 3-party for the SotFMIEvaluator.
     * @param ass Additive sharing for 2-party for the SotFMIEvaluator.
     */
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
    SotFMIParameters              params_;  /**< SotFMIParameters for the SotFMIEvaluator. */
    wm::SotWMEvaluator            wm_eval_; /**< SotWMEvaluator for the SotFMIEvaluator. */
    proto::ZeroTestEvaluator      zt_eval_; /**< ZeroTestEvaluator for the SotFMIEvaluator. */
    sharing::ReplicatedSharing3P &rss_;     /**< Replicated sharing for 3-party for the SotFMIEvaluator. */
    sharing::AdditiveSharing2P   &ass_;     /**< Additive sharing for 2-party for the SotFMIEvaluator. */
};

}    // namespace fm_index
}    // namespace ringoa

#endif    // FM_INDEX_SOTFMI_H_
