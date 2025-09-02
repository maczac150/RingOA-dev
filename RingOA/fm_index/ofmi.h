#ifndef FM_INDEX_SECURE_FMI_H_
#define FM_INDEX_SECURE_FMI_H_

#include "RingOA/protocol/zero_test.h"
#include "RingOA/wm/secure_wm.h"

namespace ringoa {
namespace fm_index {

/**
 * @brief A class to hold parameters for the SecureFMI.
 */
class SecureFMIParameters {
public:
    /**
     * @brief Default constructor for SecureFMIParameters is deleted.
     */
    SecureFMIParameters() = delete;

    /**
     * @brief Parameterized constructor for SecureFMIParameters.
     * @param database_bitsize The database size for the SecureFMIParameters.
     * @param query_size The query size for the SecureFMIParameters.
     * @param sigma The alphabet size for the SecureFMIParameters.
     * @param mode The output type for the SecureFMIParameters.
     */
    SecureFMIParameters(const uint64_t database_bitsize,
                        const uint64_t query_size,
                        const uint64_t sigma = 3)
        : query_size_(query_size),
          swm_params_(database_bitsize, sigma),
          zt_params_(database_bitsize, database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the SecureFMIParameters.
     * @param database_bitsize The database size for the SecureFMIParameters.
     * @param query_size The query size for the SecureFMIParameters.
     * @param sigma The alphabet size for the SecureFMIParameters.
     * @param mode The output type for the SecureFMIParameters.
     */

    void ReconfigureParameters(const uint64_t database_bitsize,
                               const uint64_t query_size,
                               const uint64_t sigma = 3) {
        query_size_ = query_size;
        swm_params_.ReconfigureParameters(database_bitsize, sigma);
        zt_params_.ReconfigureParameters(database_bitsize, database_bitsize);
    }

    /**
     * @brief Get the database bit size for the SecureFMIParameters.
     * @return uint64_t The database size for the SecureFMIParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return swm_params_.GetDatabaseBitSize();
    }

    /**
     * @brief Get the database size for the SecureFMIParameters.
     * @return uint64_t The database size for the SecureFMIParameters.
     */
    uint64_t GetDatabaseSize() const {
        return swm_params_.GetDatabaseSize();
    }

    /**
     * @brief Get the query size for the SecureFMIParameters.
     * @return uint64_t The query size for the SecureFMIParameters.
     */
    uint64_t GetQuerySize() const {
        return query_size_;
    }

    /**
     * @brief Get the alphabet size for the SecureFMIParameters.
     * @return uint64_t The alphabet size for the SecureFMIParameters.
     */
    uint64_t GetSigma() const {
        return swm_params_.GetSigma();
    }

    /**
     * @brief Get the SecureWMParameters for the SecureFMIParameters.
     * @return const wm::SecureWMParameters& The SecureWMParameters for the SecureFMIParameters.
     */
    const wm::SecureWMParameters GetSecureWMParameters() const {
        return swm_params_;
    }

    /**
     * @brief Get the ZeroTestParameters for the SecureFMIParameters.
     * @return const ZeroTestParameters& The ZeroTestParameters for the SecureFMIParameters.
     */
    const proto::ZeroTestParameters GetZeroTestParameters() const {
        return zt_params_;
    }

    /**
     * @brief Get the parameters info for the SecureFMIParameters.
     * @return std::string The parameters info for the SecureFMIParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "Query size: " << query_size_ << ", " << swm_params_.GetParametersInfo() << ", " << zt_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the parameters info for the SecureFMIParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                  query_size_; /**< The query size for the SecureFMIParameters. */
    wm::SecureWMParameters    swm_params_; /**< The SecureWMParameters for the SecureFMIParameters. */
    proto::ZeroTestParameters zt_params_;  /**< The ZeroTestParameters for the SecureFMIParameters. */
};

/**
 * @brief A struct to hold the keys for the SecureFMI.
 */
struct SecureFMIKey {
    uint64_t                        num_wm_keys;
    uint64_t                        num_zt_keys;
    std::vector<wm::SecureWMKey>    wm_f_keys;
    std::vector<wm::SecureWMKey>    wm_g_keys;
    std::vector<proto::ZeroTestKey> zt_keys;

    /**
     * @brief Default constructor for SecureFMIKey is deleted.
     */
    SecureFMIKey() = delete;

    /**
     * @brief Parameterized constructor for SecureFMIKey.
     * @param id The ID for the SecureFMIKey.
     * @param params The SecureFMIParameters for the SecureFMIKey.
     */
    SecureFMIKey(const uint64_t id, const SecureFMIParameters &params);

    /**
     * @brief Default destructor for SecureFMIKey.
     */
    ~SecureFMIKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of SecureFMIKey.
     */
    SecureFMIKey(const SecureFMIKey &other)            = delete;
    SecureFMIKey &operator=(const SecureFMIKey &other) = delete;

    /**
     * @brief Move constructor for SecureFMIKey.
     */
    SecureFMIKey(SecureFMIKey &&) noexcept            = default;
    SecureFMIKey &operator=(SecureFMIKey &&) noexcept = default;

    /**
     * @brief Compare two SecureFMIKey for equality.
     * @param rhs The SecureFMIKey to compare with.
     * @return bool True if the SecureFMIKey are equal, false otherwise.
     */
    bool operator==(const SecureFMIKey &rhs) const {
        return (num_wm_keys == rhs.num_wm_keys) &&
               (num_zt_keys == rhs.num_zt_keys) &&
               (wm_f_keys == rhs.wm_f_keys) &&
               (wm_g_keys == rhs.wm_g_keys) &&
               (zt_keys == rhs.zt_keys);
    }
    bool operator!=(const SecureFMIKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Calculate the size of the serialized SecureFMIKey.
     * @param buffer The buffer to store the serialized SecureFMIKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the SecureFMIKey.
     * @param buffer The buffer to store the serialized SecureFMIKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the SecureFMIKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    SecureFMIParameters params_; /**< The SecureFMIParameters for the SecureFMIKey. */
};

/**
 * @brief A class to generate keys for the SecureFMI.
 */
class SecureFMIKeyGenerator {
public:
    /**
     * @brief Default constructor for SecureFMIKeyGenerator is deleted.
     */
    SecureFMIKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for SecureFMIKeyGenerator.
     * @param params SecureWMParameters for the SecureFMIKeyGenerator.
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    SecureFMIKeyGenerator(
        const SecureFMIParameters    &params,
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
     * @brief Generate keys for the SecureFMI.
     * @return std::array<SecureFMIKey, 3> Array of generated SecureFMIKeys.
     */
    std::array<SecureFMIKey, 3> GenerateKeys() const;

private:
    SecureFMIParameters           params_; /**< SecureFMIParameters for the SecureFMIKeyGenerator. */
    wm::SecureWMKeyGenerator      wm_gen_; /**< SecureWMKeyGenerator for the SecureFMIKeyGenerator. */
    proto::ZeroTestKeyGenerator   zt_gen_; /**< ZeroTestKeyGenerator for the SecureFMIKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the SecureFMIEvaluator. */
};

/**
 * @brief A class to evaluate keys for the SecureFMI.
 */
class SecureFMIEvaluator {
public:
    /**
     * @brief Default constructor for SecureFMIEvaluator is deleted.
     */
    SecureFMIEvaluator() = delete;

    /**
     * @brief Parameterized constructor for SecureFMIEvaluator.
     * @param params SecureFMIParameters for the SecureFMIEvaluator.
     * @param rss Replicated sharing for 3-party for the SecureFMIEvaluator.
     * @param ass_prev Additive sharing for 2-party for the SecureFMIEvaluator.
     * @param ass_next Additive sharing for 2-party for the SecureFMIEvaluator.
     */
    SecureFMIEvaluator(const SecureFMIParameters    &params,
                       sharing::ReplicatedSharing3P &rss,
                       sharing::AdditiveSharing2P   &ass_prev,
                       sharing::AdditiveSharing2P   &ass_next);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path);

    void EvaluateLPM(Channels                     &chls,
                     const SecureFMIKey           &key,
                     std::vector<block>           &uv_prev,
                     std::vector<block>           &uv_next,
                     const sharing::RepShareMat64 &wm_tables,
                     const sharing::RepShareMat64 &query,
                     sharing::RepShareVec64       &result) const;

    void EvaluateLPM_Parallel(Channels                     &chls,
                              const SecureFMIKey           &key,
                              std::vector<block>           &uv_prev,
                              std::vector<block>           &uv_next,
                              const sharing::RepShareMat64 &wm_tables,
                              const sharing::RepShareMat64 &query,
                              sharing::RepShareVec64       &result) const;

private:
    SecureFMIParameters           params_;   /**< SecureFMIParameters for the SecureFMIEvaluator. */
    wm::SecureWMEvaluator         wm_eval_;  /**< SecureWMEvaluator for the SecureFMIEvaluator. */
    proto::ZeroTestEvaluator      zt_eval_;  /**< ZeroTestEvaluator for the SecureFMIEvaluator. */
    sharing::ReplicatedSharing3P &rss_;      /**< Replicated sharing for 3-party for the SecureFMIEvaluator. */
    sharing::AdditiveSharing2P   &ass_prev_; /**< Additive sharing for 2-party for the previous party. */
    sharing::AdditiveSharing2P   &ass_next_; /**< Additive sharing for 2-party for the next party. */
};

}    // namespace fm_index
}    // namespace ringoa

#endif    // FM_INDEX_SECURE_FMI_H_
