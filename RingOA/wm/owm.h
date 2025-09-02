#ifndef WM_OWM_H_
#define WM_OWM_H_

#include "RingOA/protocol/ringoa.h"

namespace ringoa {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class FMIndex;

/**
 * @brief A class to hold parameters for the OWM.
 */
class OWMParameters {
public:
    /**
     * @brief Default constructor for OWMParameters is deleted.
     */
    OWMParameters() = delete;

    /**
     * @brief Parameterized constructor for OWMParameters.
     * @param database_bitsize The database size for the OWMParameters.
     * @param sigma The alphabet size for the OWMParameters.
     * @param mode The output type for the OWMParameters.
     */
    OWMParameters(const uint64_t database_bitsize,
                  const uint64_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          sigma_(sigma),
          oa_params_(database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the OWMParameters.
     * @param database_bitsize The database size for the OWMParameters.
     * @param query_size The query size for the OWMParameters.
     * @param sigma The alphabet size for the OWMParameters.
     */
    void ReconfigureParameters(const uint64_t database_bitsize, const uint64_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        sigma_            = sigma;
        oa_params_.ReconfigureParameters(database_bitsize);
    }

    /**
     * @brief Get the database bit size for the OWMParameters.
     * @return uint64_t The database size for the OWMParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }

    /**
     * @brief Get the database size for the OWMParameters.
     * @return uint64_t The database size for the OWMParameters.
     */
    uint64_t GetDatabaseSize() const {
        return database_size_;
    }

    /**
     * @brief Get the alphabet size for the OWMParameters.
     * @return uint64_t The alphabet size for the OWMParameters.
     */
    uint64_t GetSigma() const {
        return sigma_;
    }

    /**
     * @brief Get the RingOaParameters for the OWMParameters.
     * @return const RingOaParameters& The RingOaParameters for the OWMParameters.
     */
    const proto::RingOaParameters GetOaParameters() const {
        return oa_params_;
    }

    /**
     * @brief Get the parameters info for the OWMParameters.
     * @return std::string The parameters info for the OWMParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", RingOA params: " << oa_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the details of the OWMParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                database_bitsize_; /**< The database bit size for the OWMParameters. */
    uint64_t                database_size_;    /**< The database size for the OWMParameters. */
    uint64_t                sigma_;            /**< The alphabet size for the OWMParameters. */
    proto::RingOaParameters oa_params_;        /**< The RingOaParameters for the OWMParameters. */
};

/**
 * @brief A struct to hold the OWM key.
 */
struct OWMKey {
    uint64_t                      num_oa_keys;
    std::vector<proto::RingOaKey> oa_keys;

    /**
     * @brief Default constructor for OWMKey is deleted.
     */
    OWMKey() = delete;

    /**
     * @brief Parameterized constructor for OWMKey.
     * @param id The ID for the OWMKey.
     * @param params The OWMParameters for the OWMKey.
     */
    OWMKey(const uint64_t id, const OWMParameters &params);

    /**
     * @brief Default destructor for OWMKey.
     */
    ~OWMKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of OWMKey.
     */
    OWMKey(const OWMKey &)            = delete;
    OWMKey &operator=(const OWMKey &) = delete;

    /**
     * @brief Move constructor for OWMKey.
     */
    OWMKey(OWMKey &&) noexcept            = default;
    OWMKey &operator=(OWMKey &&) noexcept = default;

    /**
     * @brief Compare two OWMKey for equality.
     * @param rhs The OWMKey to compare with.
     * @return bool True if the OWMKey are equal, false otherwise.
     */
    bool operator==(const OWMKey &rhs) const {
        return (num_oa_keys == rhs.num_oa_keys) && (oa_keys == rhs.oa_keys);
    }

    bool operator!=(const OWMKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized OWMKey.
     * @return size_t The size of the serialized OWMKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized OWMKey.
     * @return size_t The size of the serialized OWMKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Calculate the size of the serialized OWMKey.
     * @return size_t The size of the serialized OWMKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Serialize the OWMKey.
     * @param buffer The buffer to store the serialized OWMKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the key for the OWMKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    OWMParameters params_;          /**< The OWMParameters for the OWMKey. */
    size_t        serialized_size_; /**< The size of the serialized OWMKey. */
};

/**
 * @brief A class to generate keys for the OWM.
 */
class OWMKeyGenerator {
public:
    /**
     * @brief Default constructor for OWMKeyGenerator is deleted.
     */
    OWMKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for OWMKeyGenerator.
     * @param params OWMParameters for the OWMKeyGenerator.
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    OWMKeyGenerator(
        const OWMParameters          &params,
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
     * @brief Generate keys for the OWM.
     * @return std::array<OWMKey, 3> Array of generated OWMKeys.
     */
    std::array<OWMKey, 3> GenerateKeys() const;

private:
    OWMParameters                 params_; /**< OWMParameters for the OWMKeyGenerator. */
    proto::RingOaKeyGenerator     oa_gen_; /**< RingOaKeyGenerator for the OWMKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the OWMKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the OWM.
 */
class OWMEvaluator {
public:
    /**
     * @brief Default constructor for OWMEvaluator is deleted.
     */
    OWMEvaluator() = delete;

    /**
     * @brief Parameterized constructor for OWMEvaluator.
     * @param params OWMParameters for the OWMEvaluator.
     * @param rss Replicated sharing for 3-party for the RingOaEvaluator.
     * @param ass_prev Additive sharing for 2-party (previous) for the RingOaEvaluator.
     * @param ass_next Additive sharing for 2-party (next) for the RingOaEvaluator.
     */
    OWMEvaluator(const OWMParameters          &params,
                 sharing::ReplicatedSharing3P &rss,
                 sharing::AdditiveSharing2P   &ass_prev,
                 sharing::AdditiveSharing2P   &ass_next);

    const proto::RingOaEvaluator &GetRingOaEvaluator() const {
        return oa_eval_;
    }

    void EvaluateRankCF(Channels                      &chls,
                        const OWMKey                  &key,
                        std::vector<block>            &uv_prev,
                        std::vector<block>            &uv_next,
                        const sharing::RepShareMat64  &wm_tables,
                        const sharing::RepShareView64 &char_sh,
                        sharing::RepShare64           &position_sh,
                        sharing::RepShare64           &result) const;

    void EvaluateRankCF_Parallel(Channels                      &chls,
                                 const OWMKey                  &key1,
                                 const OWMKey                  &key2,
                                 std::vector<block>            &uv_prev,
                                 std::vector<block>            &uv_next,
                                 const sharing::RepShareMat64  &wm_tables,
                                 const sharing::RepShareView64 &char_sh,
                                 sharing::RepShareVec64        &position_sh,
                                 sharing::RepShareVec64        &result) const;

private:
    OWMParameters                 params_;  /**< OWMParameters for the OWMEvaluator. */
    proto::RingOaEvaluator        oa_eval_; /**< RingOaEvaluator for the OWMEvaluator. */
    sharing::ReplicatedSharing3P &rss_;     /**< Replicated sharing for 3-party for the RingOaEvaluator. */
};

}    // namespace wm
}    // namespace ringoa

#endif    // WM_OWM_H_
