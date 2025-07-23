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

/**
 * @brief A class to hold parameters for the SotWM.
 */
class SotWMParameters {
public:
    /**
     * @brief Default constructor for SotWMParameters is deleted.
     */
    SotWMParameters() = delete;

    /**
     * @brief Parameterized constructor for SotWMParameters.
     * @param database_bitsize The database size for the SotWMParameters.
     * @param sigma The alphabet size for the SotWMParameters.
     * @param mode The output type for the SotWMParameters.
     */
    SotWMParameters(const uint64_t database_bitsize,
                    const uint64_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          sigma_(sigma),
          sot_params_(database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the SotWMParameters.
     * @param database_bitsize The database size for the SotWMParameters.
     * @param query_size The query size for the SotWMParameters.
     * @param sigma The alphabet size for the SotWMParameters.
     */
    void ReconfigureParameters(const uint64_t database_bitsize, const uint64_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        sigma_            = sigma;
        sot_params_.ReconfigureParameters(database_bitsize);
    }

    /**
     * @brief Get the database bit size for the SotWMParameters.
     * @return uint64_t The database size for the SotWMParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }

    /**
     * @brief Get the database size for the SotWMParameters.
     * @return uint64_t The database size for the SotWMParameters.
     */
    uint64_t GetDatabaseSize() const {
        return database_size_;
    }

    /**
     * @brief Get the alphabet size for the SotWMParameters.
     * @return uint64_t The alphabet size for the SotWMParameters.
     */
    uint64_t GetSigma() const {
        return sigma_;
    }

    /**
     * @brief Get the SharedOtParameters for the SotWMParameters.
     * @return const SharedOtParameters& The SharedOtParameters for the SotWMParameters.
     */
    const proto::SharedOtParameters GetSotParameters() const {
        return sot_params_;
    }

    /**
     * @brief Get the parameters info for the SotWMParameters.
     * @return std::string The parameters info for the SotWMParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", SOT params: " << sot_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the details of the SotWMParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                  database_bitsize_; /**< The database bit size for the SotWMParameters. */
    uint64_t                  database_size_;    /**< The database size for the SotWMParameters. */
    uint64_t                  sigma_;            /**< The alphabet size for the SotWMParameters. */
    proto::SharedOtParameters sot_params_;       /**< The SharedOtParameters for the SotWMParameters. */
};

/**
 * @brief A struct to hold the SotWM key.
 */
struct SotWMKey {
    uint64_t                        num_sot_keys;
    std::vector<proto::SharedOtKey> sot_keys;

    /**
     * @brief Default constructor for SotWMKey is deleted.
     */
    SotWMKey() = delete;

    /**
     * @brief Parameterized constructor for SotWMKey.
     * @param id The ID for the SotWMKey.
     * @param params The SotWMParameters for the SotWMKey.
     */
    SotWMKey(const uint64_t id, const SotWMParameters &params);

    /**
     * @brief Default destructor for SotWMKey.
     */
    ~SotWMKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of SotWMKey.
     */
    SotWMKey(const SotWMKey &)            = delete;
    SotWMKey &operator=(const SotWMKey &) = delete;

    /**
     * @brief Move constructor for SotWMKey.
     */
    SotWMKey(SotWMKey &&) noexcept            = default;
    SotWMKey &operator=(SotWMKey &&) noexcept = default;

    /**
     * @brief Compare two SotWMKey for equality.
     * @param rhs The SotWMKey to compare with.
     * @return bool True if the SotWMKey are equal, false otherwise.
     */
    bool operator==(const SotWMKey &rhs) const {
        return (num_sot_keys == rhs.num_sot_keys) && (sot_keys == rhs.sot_keys);
    }

    bool operator!=(const SotWMKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized SotWMKey.
     * @return size_t The size of the serialized SotWMKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized SotWMKey.
     * @return size_t The size of the serialized SotWMKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Calculate the size of the serialized SotWMKey.
     * @return size_t The size of the serialized SotWMKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the SotWMKey.
     * @param buffer The buffer to store the serialized SotWMKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the SotWMKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    SotWMParameters params_;          /**< The SotWMParameters for the SotWMKey. */
    size_t          serialized_size_; /**< The size of the serialized SotWMKey. */
};

/**
 * @brief A class to generate keys for the SotWM.
 */
class SotWMKeyGenerator {
public:
    /**
     * @brief Default constructor for SotWMKeyGenerator is deleted.
     */
    SotWMKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for SotWMKeyGenerator.
     * @param params SotWMParameters for the SotWMKeyGenerator.
     * @param ass Additive sharing for 2-party for the SharedOtKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    SotWMKeyGenerator(
        const SotWMParameters        &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    const proto::SharedOtKeyGenerator &GetSharedOtKeyGenerator() const {
        return sot_gen_;
    }

    /**
     * @brief Generate replicated shares for the database.
     * @param fm The FMIndex used to generate the shares.
     * @return std::array<std::pair<sharing::RepShareMat64, sharing::RepShareMat64>, 3> The replicated shares for the database.
     */
    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const FMIndex &fm) const;

    /**
     * @brief Generate keys for the SotWM.
     * @return std::array<SotWMKey, 3> Array of generated SotWMKeys.
     */
    std::array<SotWMKey, 3> GenerateKeys() const;

private:
    SotWMParameters               params_;  /**< SotWMParameters for the SotWMKeyGenerator. */
    proto::SharedOtKeyGenerator   sot_gen_; /**< SharedOtKeyGenerator for the SotWMKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;     /**< Replicated sharing for 3-party for the SotWMKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the SotWM.
 */
class SotWMEvaluator {
public:
    /**
     * @brief Default constructor for SotWMEvaluator is deleted.
     */
    SotWMEvaluator() = delete;

    /**
     * @brief Parameterized constructor for SotWMEvaluator.
     * @param params SotWMParameters for the SotWMEvaluator.
     * @param rss Replicated sharing for 3-party for the SharedOtEvaluator.
     */
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
    SotWMParameters               params_;   /**< SotWMParameters for the SotWMEvaluator. */
    proto::SharedOtEvaluator      sot_eval_; /**< SharedOtEvaluator for the SotWMEvaluator. */
    sharing::ReplicatedSharing3P &rss_;      /**< Replicated sharing for 3-party for the SharedOtEvaluator. */
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_SOTWM_H_
