#ifndef WM_FSSWM_H_
#define WM_FSSWM_H_

#include "obliv_select.h"

namespace fsswm {
namespace wm {

/**
 * @brief A class to hold parameters for the FssWM.
 */
class FssWMParameters {
public:
    /**
     * @brief Default constructor for FssWMParameters is deleted.
     */
    FssWMParameters() = delete;

    /**
     * @brief Parameterized constructor for FssWMParameters.
     * @param database_bitsize The database size for the FssWMParameters.
     * @param query_size The query size for the FssWMParameters.
     * @param sigma The alphabet size for the FssWMParameters.
     */
    FssWMParameters(const uint32_t database_bitsize, const uint32_t query_size, const uint32_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          query_size_(query_size),
          sigma_(sigma),
          os_params_(database_bitsize, ShareType::kBinary) {
    }

    /**
     * @brief Reconfigure the parameters for the FssWMParameters.
     * @param database_bitsize The database size for the FssWMParameters.
     * @param query_size The query size for the FssWMParameters.
     * @param sigma The alphabet size for the FssWMParameters.
     */
    void ReconfigureParameters(const uint32_t database_bitsize, const uint32_t query_size, const uint32_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        query_size_       = query_size;
        sigma_            = sigma;
        os_params_.ReconfigureParameters(database_bitsize, ShareType::kBinary);
    }

    /**
     * @brief Get the database bit size for the FssWMParameters.
     * @return uint32_t The database size for the FssWMParameters.
     */
    uint32_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }

    /**
     * @brief Get the database size for the FssWMParameters.
     * @return uint32_t The database size for the FssWMParameters.
     */
    uint32_t GetDatabaseSize() const {
        return database_size_;
    }

    /**
     * @brief Get the query size for the FssWMParameters.
     * @return uint32_t The query size for the FssWMParameters.
     */
    uint32_t GetQuerySize() const {
        return query_size_;
    }

    /**
     * @brief Get the alphabet size for the FssWMParameters.
     * @return uint32_t The alphabet size for the FssWMParameters.
     */
    uint32_t GetSigma() const {
        return sigma_;
    }

    /**
     * @brief Get the OblivSelectParameters for the FssWMParameters.
     * @return const OblivSelectParameters& The OblivSelectParameters for the FssWMParameters.
     */
    const OblivSelectParameters GetParameters() const {
        return os_params_;
    }

    /**
     * @brief Get the parameters info for the FssWMParameters.
     * @return std::string The parameters info for the FssWMParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Query size: " << query_size_ << ", Sigma: " << sigma_ << ", OS params: " << os_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the details of the FssWMParameters.
     */
    void PrintParameters() const;

private:
    uint32_t              database_bitsize_; /**< The database bit size for the FssWMParameters. */
    uint32_t              database_size_;    /**< The database size for the FssWMParameters. */
    uint32_t              query_size_;       /**< The query size for the FssWMParameters. */
    uint32_t              sigma_;            /**< The alphabet size for the FssWMParameters. */
    OblivSelectParameters os_params_;        /**< The OblivSelectParameters for the FssWMParameters. */
};

/**
 * @brief A struct to hold the FssWM key.
 */
struct FssWMKey {
    uint32_t                    num_os_keys;
    std::vector<OblivSelectKey> os_f_keys;
    std::vector<OblivSelectKey> os_g_keys;

    /**
     * @brief Default constructor for FssWMKey is deleted.
     */
    FssWMKey() = delete;

    /**
     * @brief Parameterized constructor for FssWMKey.
     * @param id The ID for the FssWMKey.
     * @param params The FssWMParameters for the FssWMKey.
     */
    FssWMKey(const uint32_t id, const FssWMParameters &params);

    /**
     * @brief Default destructor for FssWMKey.
     */
    ~FssWMKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of FssWMKey.
     */
    FssWMKey(const FssWMKey &)            = delete;
    FssWMKey &operator=(const FssWMKey &) = delete;

    /**
     * @brief Move constructor for FssWMKey.
     */
    FssWMKey(FssWMKey &&) noexcept            = default;
    FssWMKey &operator=(FssWMKey &&) noexcept = default;

    /**
     * @brief Compare two FssWMKey for equality.
     * @param rhs The FssWMKey to compare with.
     * @return bool True if the FssWMKey are equal, false otherwise.
     */
    bool operator==(const FssWMKey &rhs) const {
        return (num_os_keys == rhs.num_os_keys) && (os_f_keys == rhs.os_f_keys) && (os_g_keys == rhs.os_g_keys);
    }

    bool operator!=(const FssWMKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Calculate the size of the serialized FssWMKey.
     * @return size_t The size of the serialized FssWMKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the FssWMKey.
     * @param buffer The buffer to store the serialized FssWMKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the FssWMKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    FssWMParameters params_; /**< The FssWMParameters for the FssWMKey. */
};

/**
 * @brief A class to generate keys for the FssWM.
 */
class FssWMKeyGenerator {
public:
    /**
     * @brief Default constructor for FssWMKeyGenerator is deleted.
     */
    FssWMKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for FssWMKeyGenerator.
     * @param params FssWMParameters for the FssWMKeyGenerator.
     * @param ass Additive sharing for 2-party for the OblivSelectKeyGenerator.
     * @param bss Binary sharing for 2-party for the OblivSelectKeyGenerator.
     */
    FssWMKeyGenerator(
        const FssWMParameters      &params,
        sharing::AdditiveSharing2P &ass,
        sharing::BinarySharing2P   &bss);

    /**
     * @brief Generate keys for the FssWM.
     * @return std::array<FssWMKey, 3> Array of generated FssWMKeys.
     */
    std::array<FssWMKey, 3> GenerateKeys() const;

private:
    FssWMParameters             params_; /**< FssWMParameters for the FssWMKeyGenerator. */
    OblivSelectKeyGenerator     gen_;    /**< OblivSelectKeyGenerator for the FssWMKeyGenerator. */
    sharing::AdditiveSharing2P &ass_;    /**< Additive sharing for 2-party for the OblivSelectKeyGenerator. */
    sharing::BinarySharing2P   &bss_;    /**< Binary sharing for 2-party for the OblivSelectKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the FssWM.
 */
class FssWMEvaluator {
public:
    /**
     * @brief Default constructor for FssWMEvaluator is deleted.
     */
    FssWMEvaluator() = delete;

    /**
     * @brief Parameterized constructor for FssWMEvaluator.
     * @param params FssWMParameters for the FssWMEvaluator.
     * @param rss Replicated sharing for 3-party for the OblivSelectEvaluator.
     * @param brss Binary replicated sharing for 3-party for the OblivSelectEvaluator.
     */
    FssWMEvaluator(const FssWMParameters              &params,
                   sharing::ReplicatedSharing3P       &rss,
                   sharing::BinaryReplicatedSharing3P &brss);

    void Evaluate(sharing::Channels                      &chls,
                  const FssWMKey                         &key,
                  const std::vector<sharing::SharesPair> &wm_table,
                  const sharing::SharesPair              &query,
                  sharing::SharesPair                    &result) const;

private:
    FssWMParameters                     params_; /**< FssWMParameters for the FssWMEvaluator. */
    OblivSelectEvaluator                eval_;   /**< OblivSelectEvaluator for the FssWMEvaluator. */
    sharing::ReplicatedSharing3P       &rss_;    /**< Replicated sharing for 3-party for the FssWMEvaluator. */
    sharing::BinaryReplicatedSharing3P &brss_;   /**< Binary replicated sharing for 3-party for the OblivSelectEvaluator. */
};

}    // namespace wm
}    // namespace fsswm

#endif
