#ifndef WM_SECURE_WM_H_
#define WM_SECURE_WM_H_

#include "RingOA/protocol/ringoa.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class FMIndex;

/**
 * @brief A class to hold parameters for the SecureWM.
 */
class SecureWMParameters {
public:
    /**
     * @brief Default constructor for SecureWMParameters is deleted.
     */
    SecureWMParameters() = delete;

    /**
     * @brief Parameterized constructor for SecureWMParameters.
     * @param database_bitsize The database size for the SecureWMParameters.
     * @param sigma The alphabet size for the SecureWMParameters.
     * @param mode The output type for the SecureWMParameters.
     */
    SecureWMParameters(const uint64_t database_bitsize,
                       const uint64_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          sigma_(sigma),
          oa_params_(database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the SecureWMParameters.
     * @param database_bitsize The database size for the SecureWMParameters.
     * @param query_size The query size for the SecureWMParameters.
     * @param sigma The alphabet size for the SecureWMParameters.
     */
    void ReconfigureParameters(const uint64_t database_bitsize, const uint64_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        sigma_            = sigma;
        oa_params_.ReconfigureParameters(database_bitsize);
    }

    /**
     * @brief Get the database bit size for the SecureWMParameters.
     * @return uint64_t The database size for the SecureWMParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }

    /**
     * @brief Get the database size for the SecureWMParameters.
     * @return uint64_t The database size for the SecureWMParameters.
     */
    uint64_t GetDatabaseSize() const {
        return database_size_;
    }

    /**
     * @brief Get the alphabet size for the SecureWMParameters.
     * @return uint64_t The alphabet size for the SecureWMParameters.
     */
    uint64_t GetSigma() const {
        return sigma_;
    }

    /**
     * @brief Get the RingOaParameters for the SecureWMParameters.
     * @return const RingOaParameters& The RingOaParameters for the SecureWMParameters.
     */
    const proto::RingOaParameters GetOaParameters() const {
        return oa_params_;
    }

    /**
     * @brief Get the parameters info for the SecureWMParameters.
     * @return std::string The parameters info for the SecureWMParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", RingOA params: " << oa_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the details of the SecureWMParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                database_bitsize_; /**< The database bit size for the SecureWMParameters. */
    uint64_t                database_size_;    /**< The database size for the SecureWMParameters. */
    uint64_t                sigma_;            /**< The alphabet size for the SecureWMParameters. */
    proto::RingOaParameters oa_params_;        /**< The RingOaParameters for the SecureWMParameters. */
};

/**
 * @brief A struct to hold the SecureWM key.
 */
struct SecureWMKey {
    uint64_t                      num_oa_keys;
    std::vector<proto::RingOaKey> oa_keys;

    /**
     * @brief Default constructor for SecureWMKey is deleted.
     */
    SecureWMKey() = delete;

    /**
     * @brief Parameterized constructor for SecureWMKey.
     * @param id The ID for the SecureWMKey.
     * @param params The SecureWMParameters for the SecureWMKey.
     */
    SecureWMKey(const uint64_t id, const SecureWMParameters &params);

    /**
     * @brief Default destructor for SecureWMKey.
     */
    ~SecureWMKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of SecureWMKey.
     */
    SecureWMKey(const SecureWMKey &)            = delete;
    SecureWMKey &operator=(const SecureWMKey &) = delete;

    /**
     * @brief Move constructor for SecureWMKey.
     */
    SecureWMKey(SecureWMKey &&) noexcept            = default;
    SecureWMKey &operator=(SecureWMKey &&) noexcept = default;

    /**
     * @brief Compare two SecureWMKey for equality.
     * @param rhs The SecureWMKey to compare with.
     * @return bool True if the SecureWMKey are equal, false otherwise.
     */
    bool operator==(const SecureWMKey &rhs) const {
        return (num_oa_keys == rhs.num_oa_keys) && (oa_keys == rhs.oa_keys);
    }

    bool operator!=(const SecureWMKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized SecureWMKey.
     * @return size_t The size of the serialized SecureWMKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized SecureWMKey.
     * @return size_t The size of the serialized SecureWMKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Calculate the size of the serialized SecureWMKey.
     * @return size_t The size of the serialized SecureWMKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the SecureWMKey.
     * @param buffer The buffer to store the serialized SecureWMKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the SecureWMKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    SecureWMParameters params_;          /**< The SecureWMParameters for the SecureWMKey. */
    size_t             serialized_size_; /**< The size of the serialized SecureWMKey. */
};

/**
 * @brief A class to generate keys for the SecureWM.
 */
class SecureWMKeyGenerator {
public:
    /**
     * @brief Default constructor for SecureWMKeyGenerator is deleted.
     */
    SecureWMKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for SecureWMKeyGenerator.
     * @param params SecureWMParameters for the SecureWMKeyGenerator.
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    SecureWMKeyGenerator(
        const SecureWMParameters     &params,
        sharing::AdditiveSharing2P   &ass,
        sharing::ReplicatedSharing3P &rss);

    const proto::RingOaKeyGenerator &GetRingOaKeyGenerator() const {
        return oa_gen_;
    }

    /**
     * @brief Generate replicated shares for the database.
     * @param fm The FMIndex used to generate the shares.
     * @return std::array<std::pair<sharing::RepShareMat64, sharing::RepShareMat64>, 3> The replicated shares for the database.
     */
    std::array<sharing::RepShareMat64, 3> GenerateDatabaseU64Share(const FMIndex &fm) const;

    /**
     * @brief Generate keys for the SecureWM.
     * @return std::array<SecureWMKey, 3> Array of generated SecureWMKeys.
     */
    std::array<SecureWMKey, 3> GenerateKeys() const;

private:
    SecureWMParameters            params_; /**< SecureWMParameters for the SecureWMKeyGenerator. */
    proto::RingOaKeyGenerator     oa_gen_; /**< RingOaKeyGenerator for the SecureWMKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the SecureWMKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the SecureWM.
 */
class SecureWMEvaluator {
public:
    /**
     * @brief Default constructor for SecureWMEvaluator is deleted.
     */
    SecureWMEvaluator() = delete;

    /**
     * @brief Parameterized constructor for SecureWMEvaluator.
     * @param params SecureWMParameters for the SecureWMEvaluator.
     * @param rss Replicated sharing for 3-party for the RingOaEvaluator.
     * @param ass_prev Additive sharing for 2-party (previous) for the RingOaEvaluator.
     * @param ass_next Additive sharing for 2-party (next) for the RingOaEvaluator.
     */
    SecureWMEvaluator(const SecureWMParameters     &params,
                      sharing::ReplicatedSharing3P &rss,
                      sharing::AdditiveSharing2P   &ass_prev,
                      sharing::AdditiveSharing2P   &ass_next);

    const proto::RingOaEvaluator &GetRingOaEvaluator() const {
        return oa_eval_;
    }

    void EvaluateRankCF(Channels                      &chls,
                        const SecureWMKey             &key,
                        std::vector<block>            &uv_prev,
                        std::vector<block>            &uv_next,
                        const sharing::RepShareMat64  &wm_tables,
                        const sharing::RepShareView64 &char_sh,
                        sharing::RepShare64           &position_sh,
                        sharing::RepShare64           &result) const;

    void EvaluateRankCF_Parallel(Channels                      &chls,
                                 const SecureWMKey             &key1,
                                 const SecureWMKey             &key2,
                                 std::vector<block>            &uv_prev,
                                 std::vector<block>            &uv_next,
                                 const sharing::RepShareMat64  &wm_tables,
                                 const sharing::RepShareView64 &char_sh,
                                 sharing::RepShareVec64        &position_sh,
                                 sharing::RepShareVec64        &result) const;

private:
    SecureWMParameters            params_;  /**< SecureWMParameters for the SecureWMEvaluator. */
    proto::RingOaEvaluator        oa_eval_; /**< RingOaEvaluator for the SecureWMEvaluator. */
    sharing::ReplicatedSharing3P &rss_;     /**< Replicated sharing for 3-party for the RingOaEvaluator. */
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_SECURE_WM_H_
