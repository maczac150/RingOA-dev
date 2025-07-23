#ifndef PROTOCOL_RINGOA_H_
#define PROTOCOL_RINGOA_H_

#include "FssWM/fss/dpf_eval.h"
#include "FssWM/fss/dpf_gen.h"
#include "FssWM/fss/dpf_key.h"
#include "FssWM/sharing/share_types.h"

namespace fsswm {

class Channels;

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

/**
 * @brief A class to hold params for the Oblivious Selection.
 */
class RingOaParameters {
public:
    /**
     * @brief Default constructor for RingOaParameters is deleted.
     */
    RingOaParameters() = delete;

    /**
     * @brief Parameterized constructor for RingOaParameters.
     * @param d The input bitsize.
     */
    explicit RingOaParameters(const uint64_t d)
        : params_(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive) {
    }

    /**
     *
     * @brief Reconfigure the parameters for the Zero Test technique.
     * @param d The input bitsize.
     */
    void ReconfigureParameters(const uint64_t d) {
        params_.ReconfigureParameters(d, 1, fss::kOptimizedEvalType, fss::OutputType::kShiftedAdditive);
    }

    /**
     * @brief Get the database size for the RingOaParameters.
     * @return uint64_t The input bitsize of DPF parameters.
     */
    uint64_t GetDatabaseSize() const {
        return params_.GetInputBitsize();
    }

    /**
     * @brief Get the DpfParameters for the RingOaParameters.
     * @return fss::dpf::DpfParameters The DpfParameters for the RingOaParameters.
     */
    const fss::dpf::DpfParameters GetParameters() const {
        return params_;
    }

    /**
     * @brief Get the string representation of the DpfParameters.
     * @return std::string The string representation of the DpfParameters.
     */
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }

    /**
     * @brief Print the details of the DpfParameters.
     */
    void PrintParameters() const;

private:
    fss::dpf::DpfParameters params_; /**< DpfParameters for the RingOaParameters. */
};

/**
 * @brief A struct representing a key for the Oblivious Selection.
 */
struct RingOaKey {
    uint64_t         party_id;      /**< The ID of the party associated with the key. */
    fss::dpf::DpfKey key_from_prev; /**< The DpfKey received from the previous party. */
    fss::dpf::DpfKey key_from_next; /**< The DpfKey received from the next party. */
    uint64_t         rsh_from_prev; /**< The share r received from the previous party. */
    uint64_t         rsh_from_next; /**< The share r received from the next party. */
    uint64_t         wsh_from_prev; /**< The share w received from the previous party. */
    uint64_t         wsh_from_next; /**< The share w received from the next party. */

    /**
     * @brief Default constructor for RingOaKey is deleted.
     */
    RingOaKey() = delete;

    /**
     * @brief Parameterized constructor for RingOaKey.
     * @param id The ID of the party associated with the key.
     * @param params RingOaParameters for the RingOaKey.
     */
    RingOaKey(const uint64_t id, const RingOaParameters &params);

    /**
     * @brief Destructor for RingOaKey.
     */
    ~RingOaKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of RingOaKey.
     */
    RingOaKey(const RingOaKey &)            = delete;
    RingOaKey &operator=(const RingOaKey &) = delete;

    /**
     * @brief Move constructor for RingOaKey.
     */
    RingOaKey(RingOaKey &&)            = default;
    RingOaKey &operator=(RingOaKey &&) = default;

    /**
     * @brief Equality operator for RingOaKey.
     * @param rhs The RingOaKey to compare against.
     * @return True if the RingOaKey is equal, false otherwise.
     */
    bool operator==(const RingOaKey &rhs) const {
        return (party_id == rhs.party_id) && (key_from_prev == rhs.key_from_prev) && (key_from_next == rhs.key_from_next) &&
               (rsh_from_prev == rhs.rsh_from_prev) && (rsh_from_next == rhs.rsh_from_next) &&
               (wsh_from_prev == rhs.wsh_from_prev) && (wsh_from_next == rhs.wsh_from_next);
    }

    bool operator!=(const RingOaKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized RingOaKey.
     * @return size_t The size of the serialized RingOaKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized RingOaKey.
     * @return size_t The size of the serialized RingOaKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the RingOaKey.
     * @param buffer The buffer to store the serialized RingOaKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the RingOaKey.
     * @param buffer The buffer to store the serialized RingOaKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the RingOaKey.
     * @param detailed Flag to print the detailed RingOaKey.
     */
    void PrintKey(const bool detailed = false) const;

private:
    RingOaParameters params_;          /**< RingOaParameters for the RingOaKey. */
    size_t           serialized_size_; /**< The size of the serialized RingOaKey. */
};

/**
 * @brief A class to generate keys for the Oblivious Selection.
 */
class RingOaKeyGenerator {
public:
    /**
     * @brief Default constructor for RingOaKeyGenerator is deleted.
     */
    RingOaKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for RingOaKeyGenerator.
     * @param params RingOaParameters for the RingOaKeyGenerator.
     * @param ass Additive sharing for 2-party for the RingOaKeyGenerator.
     */
    RingOaKeyGenerator(const RingOaParameters     &params,
                       sharing::AdditiveSharing2P &ass);

    /**
     * @brief Offline setup for the RingOaKeyGenerator.
     * @param num_selection The number of selections for the RingOaKeyGenerator.
     * @param file_path The file path for the RingOaKeyGenerator.
     */
    void OfflineSetUp(const uint64_t num_selection, const std::string &file_path) const;

    /**
     * @brief Generate keys for the Oblivious Selection.
     * @return std::array<RingOaKey, 3> The array of RingOaKey for the Oblivious Selection.
     */
    std::array<RingOaKey, 3> GenerateKeys() const;

private:
    RingOaParameters            params_; /**< RingOaParameters for the RingOaKeyGenerator. */
    fss::dpf::DpfKeyGenerator   gen_;    /**< DPF key generator for the RingOaKeyGenerator. */
    sharing::AdditiveSharing2P &ass_;    /**< Additive sharing for 2-party for the RingOaKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the Oblivious Selection.
 */
class RingOaEvaluator {
public:
    /**
     * @brief Default constructor for RingOaEvaluator is deleted.
     */
    RingOaEvaluator() = delete;

    /**
     * @brief Parameterized constructor for RingOaEvaluator.
     * @param params RingOaParameters for the RingOaEvaluator.
     * @param rss Replicated sharing for 3-party for the RingOaEvaluator.
     * @param ass_prev Additive sharing for 2-party for the RingOaEvaluator.
     * @param ass_next Additive sharing for 2-party for the RingOaEvaluator.
     */
    RingOaEvaluator(const RingOaParameters       &params,
                    sharing::ReplicatedSharing3P &rss,
                    sharing::AdditiveSharing2P   &ass_prev,
                    sharing::AdditiveSharing2P   &ass_next);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path) const;

    void Evaluate(Channels                      &chls,
                  const RingOaKey               &key,
                  std::vector<block>            &uv_prev,
                  std::vector<block>            &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const RingOaKey               &key1,
                           const RingOaKey               &key2,
                           std::vector<block>            &uv_prev,
                           std::vector<block>            &uv_next,
                           const sharing::RepShareView64 &database,
                           const sharing::RepShareVec64  &index,
                           sharing::RepShareVec64        &result) const;

    std::pair<uint64_t, uint64_t> EvaluateFullDomainThenDotProduct(
        const uint64_t                 party_id,
        const fss::dpf::DpfKey        &key_from_prev,
        const fss::dpf::DpfKey        &key_from_next,
        std::vector<block>            &uv_prev,
        std::vector<block>            &uv_next,
        const sharing::RepShareView64 &database,
        const uint64_t                 pr_prev,
        const uint64_t                 pr_next) const;

private:
    RingOaParameters              params_;   /**< RingOaParameters for the RingOaEvaluator. */
    fss::dpf::DpfEvaluator        eval_;     /**< DPF evaluator for the RingOaEvaluator. */
    sharing::ReplicatedSharing3P &rss_;      /**< Replicated sharing for 3-party for the RingOaEvaluator. */
    sharing::AdditiveSharing2P   &ass_prev_; /**< Additive sharing for 2-party for the RingOaEvaluator. */
    sharing::AdditiveSharing2P   &ass_next_; /**< Additive sharing for 2-party for the RingOaEvaluator. */

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructMaskedValue(
        Channels                  &chls,
        const RingOaKey           &key,
        const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructMaskedValue(
        Channels                     &chls,
        const RingOaKey              &key1,
        const RingOaKey              &key2,
        const sharing::RepShareVec64 &index) const;
};

}    // namespace proto
}    // namespace fsswm

#endif    // PROTOCOL_RINGOA_H_
