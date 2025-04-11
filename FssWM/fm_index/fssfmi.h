#ifndef FM_INDEX_FSSFMI_H_
#define FM_INDEX_FSSFMI_H_

#include "FssWM/wm/fsswm.h"
#include "zero_test.h"

namespace fsswm {
namespace fm_index {

/**
 * @brief A class to hold parameters for the FssFMI.
 */
class FssFMIParameters {
public:
    /**
     * @brief Default constructor for FssFMIParameters is deleted.
     */
    FssFMIParameters() = delete;

    /**
     * @brief Parameterized constructor for FssFMIParameters.
     * @param database_bitsize The database size for the FssFMIParameters.
     * @param query_size The query size for the FssFMIParameters.
     * @param type The type of sharing for the FssFMIParameters.
     * @param sigma The alphabet size for the FssFMIParameters.
     */
    FssFMIParameters(const uint32_t database_bitsize, const uint32_t query_size, const ShareType type, const uint32_t sigma = 3)
        : query_size_(query_size),
          fsswm_params_(database_bitsize, type, sigma),
          zt_params_(database_bitsize, type) {
    }

    /**
     * @brief Reconfigure the parameters for the FssFMIParameters.
     * @param database_bitsize The database size for the FssFMIParameters.
     * @param query_size The query size for the FssFMIParameters.
     * @param type The type of sharing for the FssFMIParameters.
     * @param sigma The alphabet size for the FssFMIParameters.
     */

    void ReconfigureParameters(const uint32_t database_bitsize, const uint32_t query_size, const ShareType type, const uint32_t sigma = 3) {
        query_size_ = query_size;
        fsswm_params_.ReconfigureParameters(database_bitsize, type, sigma);
        zt_params_.ReconfigureParameters(database_bitsize, type);
    }

    /**
     * @brief Get the database bit size for the FssFMIParameters.
     * @return uint32_t The database size for the FssFMIParameters.
     */
    uint32_t GetDatabaseBitSize() const {
        return fsswm_params_.GetDatabaseBitSize();
    }

    /**
     * @brief Get the database size for the FssFMIParameters.
     * @return uint32_t The database size for the FssFMIParameters.
     */
    uint32_t GetDatabaseSize() const {
        return fsswm_params_.GetDatabaseSize();
    }

    /**
     * @brief Get the query size for the FssFMIParameters.
     * @return uint32_t The query size for the FssFMIParameters.
     */
    uint32_t GetQuerySize() const {
        return query_size_;
    }

    /**
     * @brief Get the alphabet size for the FssFMIParameters.
     * @return uint32_t The alphabet size for the FssFMIParameters.
     */
    uint32_t GetSigma() const {
        return fsswm_params_.GetSigma();
    }

    /**
     * @brief Get the FssWMParameters for the FssFMIParameters.
     * @return const wm::FssWMParameters& The FssWMParameters for the FssFMIParameters.
     */
    const wm::FssWMParameters GetFssWMParameters() const {
        return fsswm_params_;
    }

    /**
     * @brief Get the ZeroTestParameters for the FssFMIParameters.
     * @return const ZeroTestParameters& The ZeroTestParameters for the FssFMIParameters.
     */
    const ZeroTestParameters GetZeroTestParameters() const {
        return zt_params_;
    }

    /**
     * @brief Get the parameters info for the FssFMIParameters.
     * @return std::string The parameters info for the FssFMIParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "Query size: " << query_size_ << ", " << fsswm_params_.GetParametersInfo() << ", " << zt_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the parameters info for the FssFMIParameters.
     */
    void PrintParameters() const;

private:
    uint32_t            query_size_;   /**< The query size for the FssFMIParameters. */
    wm::FssWMParameters fsswm_params_; /**< The FssWMParameters for the FssFMIParameters. */
    ZeroTestParameters  zt_params_;    /**< The ZeroTestParameters for the FssFMIParameters. */
};

/**
 * @brief A struct to hold the keys for the FssFMI.
 */
struct FssFMIKey {
    uint32_t                  num_wm_keys;
    uint32_t                  num_zt_keys;
    std::vector<wm::FssWMKey> wm_keys;
    std::vector<ZeroTestKey>  zt_keys;

    /**
     * @brief Default constructor for FssFMIKey is deleted.
     */
    FssFMIKey() = delete;

    /**
     * @brief Parameterized constructor for FssFMIKey.
     * @param id The ID for the FssFMIKey.
     * @param params The FssFMIParameters for the FssFMIKey.
     */
    FssFMIKey(const uint32_t id, const FssFMIParameters &params);

    /**
     * @brief Default destructor for FssFMIKey.
     */
    ~FssFMIKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of FssFMIKey.
     */
    FssFMIKey(const FssFMIKey &other)            = delete;
    FssFMIKey &operator=(const FssFMIKey &other) = delete;

    /**
     * @brief Move constructor for FssFMIKey.
     */
    FssFMIKey(FssFMIKey &&) noexcept            = default;
    FssFMIKey &operator=(FssFMIKey &&) noexcept = default;

    /**
     * @brief Compare two FssFMIKey for equality.
     * @param rhs The FssFMIKey to compare with.
     * @return bool True if the FssFMIKey are equal, false otherwise.
     */
    bool operator==(const FssFMIKey &rhs) const {
        return (num_wm_keys == rhs.num_wm_keys) && (num_zt_keys == rhs.num_zt_keys) && (wm_keys == rhs.wm_keys) && (zt_keys == rhs.zt_keys);
    }
    bool operator!=(const FssFMIKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Calculate the size of the serialized FssFMIKey.
     * @return size_t The size of the serialized FssFMIKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the FssFMIKey.
     * @param buffer The buffer to store the serialized FssFMIKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the FssFMIKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    FssFMIParameters params_; /**< The FssFMIParameters for the FssFMIKey. */
};

/**
 * @brief A class to generate keys for the FssFMI.
 */
class FssFMIKeyGenerator {
public:
    /**
     * @brief Default constructor for FssFMIKeyGenerator is deleted.
     */
    FssFMIKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for FssFMIKeyGenerator.
     * @param params FssWMParameters for the FssFMIKeyGenerator.
     * @param ass Additive sharing for 2-party for the OblivSelectKeyGenerator.
     * @param bss Binary sharing for 2-party for the OblivSelectKeyGenerator.
     * @param brss Binary replicated sharing for 3-party for the sharing.
     */
    FssFMIKeyGenerator(
        const FssFMIParameters             &params,
        sharing::AdditiveSharing2P         &ass,
        sharing::BinarySharing2P           &bss,
        sharing::BinaryReplicatedSharing3P &brss);

    /**
     * @brief Generate shares for the database.
     * @param fm The FMIndex used to generate the shares.
     * @return std::array<std::vector<sharing::RepShareVec>, 3> The generated shares for the database.
     */
    std::array<std::pair<sharing::RepShareMat, sharing::RepShareMat>, 3> GenerateDatabaseShare(const wm::FMIndex &fm);

    /**
     * @brief Generate shares for the query.
     * @param fm The FMIndex used to generate the shares.
     * @param query The query to generate shares for.
     * @return std::array<sharing::RepShareVec, 3> The generated shares for the query.
     */
    std::array<sharing::RepShareMat, 3> GenerateQueryShare(const wm::FMIndex &fm, std::string &query);

    /**
     * @brief Generate keys for the FssFMI.
     * @return std::array<FssFMIKey, 3> Array of generated FssFMIKeys.
     */
    std::array<FssFMIKey, 3> GenerateKeys() const;

private:
    FssFMIParameters                    params_; /**< FssFMIParameters for the FssFMIKeyGenerator. */
    wm::FssWMKeyGenerator               wm_gen_; /**< FssWMKeyGenerator for the FssFMIKeyGenerator. */
    ZeroTestKeyGenerator                zt_gen_; /**< ZeroTestKeyGenerator for the FssFMIKeyGenerator. */
    sharing::BinaryReplicatedSharing3P &brss_;   /**< Binary replicated sharing for 3-party for the FssFMIEvaluator. */
};

/**
 * @brief A class to evaluate keys for the FssFMI.
 */
class FssFMIEvaluator {
public:
    /**
     * @brief Default constructor for FssFMIEvaluator is deleted.
     */
    FssFMIEvaluator() = delete;

    FssFMIEvaluator(const FssFMIParameters             &params,
                    sharing::ReplicatedSharing3P       &rss,
                    sharing::BinaryReplicatedSharing3P &brss);

    void EvaluateLPM(sharing::Channels          &chls,
                     const FssFMIKey            &key,
                     const sharing::RepShareMat &wm_table0,
                     const sharing::RepShareMat &wm_table1,
                     const sharing::RepShareMat &query,
                     sharing::RepShareVec       &result) const;

private:
    FssFMIParameters                    params_;  /**< FssFMIParameters for the FssFMIEvaluator. */
    wm::FssWMEvaluator                  wm_eval_; /**< FssWMEvaluator for the FssFMIEvaluator. */
    ZeroTestEvaluator                   zt_eval_; /**< ZeroTestEvaluator for the FssFMIEvaluator. */
    sharing::ReplicatedSharing3P       &rss_;     /**< Replicated sharing for 3-party for the FssFMIEvaluator. */
    sharing::BinaryReplicatedSharing3P &brss_;    /**< Binary replicated sharing for 3-party for the FssFMIEvaluator. */
};

}    // namespace fm_index
}    // namespace fsswm

#endif    // FM_INDEX_FSSFMI_H_
