#ifndef WM_FSSWM_H_
#define WM_FSSWM_H_

#include "FssWM/protocol/ringoa.h"

namespace fsswm {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace wm {

class FMIndex;

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
     * @param sigma The alphabet size for the FssWMParameters.
     * @param mode The output type for the FssWMParameters.
     */
    FssWMParameters(const uint64_t database_bitsize,
                    const uint64_t sigma = 3)
        : database_bitsize_(database_bitsize),
          database_size_(1U << database_bitsize),
          sigma_(sigma),
          oa_params_(database_bitsize) {
    }

    /**
     * @brief Reconfigure the parameters for the FssWMParameters.
     * @param database_bitsize The database size for the FssWMParameters.
     * @param query_size The query size for the FssWMParameters.
     * @param sigma The alphabet size for the FssWMParameters.
     */
    void ReconfigureParameters(const uint64_t database_bitsize, const uint64_t sigma = 3) {
        database_bitsize_ = database_bitsize;
        database_size_    = 1U << database_bitsize;
        sigma_            = sigma;
        oa_params_.ReconfigureParameters(database_bitsize);
    }

    /**
     * @brief Get the database bit size for the FssWMParameters.
     * @return uint64_t The database size for the FssWMParameters.
     */
    uint64_t GetDatabaseBitSize() const {
        return database_bitsize_;
    }

    /**
     * @brief Get the database size for the FssWMParameters.
     * @return uint64_t The database size for the FssWMParameters.
     */
    uint64_t GetDatabaseSize() const {
        return database_size_;
    }

    /**
     * @brief Get the alphabet size for the FssWMParameters.
     * @return uint64_t The alphabet size for the FssWMParameters.
     */
    uint64_t GetSigma() const {
        return sigma_;
    }

    /**
     * @brief Get the RingOaParameters for the FssWMParameters.
     * @return const RingOaParameters& The RingOaParameters for the FssWMParameters.
     */
    const proto::RingOaParameters GetOaParameters() const {
        return oa_params_;
    }

    /**
     * @brief Get the parameters info for the FssWMParameters.
     * @return std::string The parameters info for the FssWMParameters.
     */
    std::string GetParametersInfo() const {
        std::ostringstream oss;
        oss << "DB size: " << database_bitsize_ << ", Sigma: " << sigma_ << ", RingOA params: " << oa_params_.GetParametersInfo();
        return oss.str();
    }

    /**
     * @brief Print the details of the FssWMParameters.
     */
    void PrintParameters() const;

private:
    uint64_t                database_bitsize_; /**< The database bit size for the FssWMParameters. */
    uint64_t                database_size_;    /**< The database size for the FssWMParameters. */
    uint64_t                sigma_;            /**< The alphabet size for the FssWMParameters. */
    proto::RingOaParameters oa_params_;        /**< The RingOaParameters for the FssWMParameters. */
};

/**
 * @brief A struct to hold the FssWM key.
 */
struct FssWMKey {
    uint64_t                      num_oa_keys;
    std::vector<proto::RingOaKey> oa_keys;

    /**
     * @brief Default constructor for FssWMKey is deleted.
     */
    FssWMKey() = delete;

    /**
     * @brief Parameterized constructor for FssWMKey.
     * @param id The ID for the FssWMKey.
     * @param params The FssWMParameters for the FssWMKey.
     */
    FssWMKey(const uint64_t id, const FssWMParameters &params);

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
        return (num_oa_keys == rhs.num_oa_keys) && (oa_keys == rhs.oa_keys);
    }

    bool operator!=(const FssWMKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized FssWMKey.
     * @return size_t The size of the serialized FssWMKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized FssWMKey.
     * @return size_t The size of the serialized FssWMKey.
     */
    size_t CalculateSerializedSize() const;

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
    FssWMParameters params_;          /**< The FssWMParameters for the FssWMKey. */
    size_t          serialized_size_; /**< The size of the serialized FssWMKey. */
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
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     * @param rss Replicated sharing for 3-party for the sharing.
     */
    FssWMKeyGenerator(
        const FssWMParameters        &params,
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
     * @brief Generate keys for the FssWM.
     * @return std::array<FssWMKey, 3> Array of generated FssWMKeys.
     */
    std::array<FssWMKey, 3> GenerateKeys() const;

private:
    FssWMParameters               params_; /**< FssWMParameters for the FssWMKeyGenerator. */
    proto::RingOaKeyGenerator     oa_gen_; /**< RingOaKeyGenerator for the FssWMKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the FssWMKeyGenerator. */
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
     * @param rss Replicated sharing for 3-party for the RingOaEvaluator.
     * @param ass_prev Additive sharing for 2-party (previous) for the RingOaEvaluator.
     * @param ass_next Additive sharing for 2-party (next) for the RingOaEvaluator.
     */
    FssWMEvaluator(const FssWMParameters        &params,
                   sharing::ReplicatedSharing3P &rss,
                   sharing::AdditiveSharing2P   &ass_prev,
                   sharing::AdditiveSharing2P   &ass_next);

    const proto::RingOaEvaluator &GetRingOaEvaluator() const {
        return oa_eval_;
    }

    void EvaluateRankCF(Channels                      &chls,
                        const FssWMKey                &key,
                        std::vector<block>            &uv_prev,
                        std::vector<block>            &uv_next,
                        const sharing::RepShareMat64  &wm_tables,
                        const sharing::RepShareView64 &char_sh,
                        sharing::RepShare64           &position_sh,
                        sharing::RepShare64           &result) const;

    void EvaluateRankCF_Parallel(Channels                      &chls,
                                 const FssWMKey                &key1,
                                 const FssWMKey                &key2,
                                 std::vector<block>            &uv_prev,
                                 std::vector<block>            &uv_next,
                                 const sharing::RepShareMat64  &wm_tables,
                                 const sharing::RepShareView64 &char_sh,
                                 sharing::RepShareVec64        &position_sh,
                                 sharing::RepShareVec64        &result) const;

private:
    FssWMParameters               params_;  /**< FssWMParameters for the FssWMEvaluator. */
    proto::RingOaEvaluator        oa_eval_; /**< RingOaEvaluator for the FssWMEvaluator. */
    sharing::ReplicatedSharing3P &rss_;     /**< Replicated sharing for 3-party for the RingOaEvaluator. */
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_FSSWM_H_
