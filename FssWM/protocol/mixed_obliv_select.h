#ifndef PROTOCOL_MIXED_OBLIV_SELECT_H_
#define PROTOCOL_MIXED_OBLIV_SELECT_H_

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
class MixedOblivSelectParameters {
public:
    /**
     * @brief Default constructor for MixedOblivSelectParameters is deleted.
     */
    MixedOblivSelectParameters() = delete;

    /**
     * @brief Parameterized constructor for MixedOblivSelectParameters.
     * @param d The input bitsize.
     */
    explicit MixedOblivSelectParameters(const uint64_t d)
        : params_(d, 1, fss::EvalType::kIterSingleBatch, fss::OutputType::kShiftedAdditive) {
    }

    /**
     * @brief Reconfigure the parameters for the Zero Test technique.
     * @param d The input bitsize.
     */
    void ReconfigureParameters(const uint64_t d) {
        params_.ReconfigureParameters(d, 1, fss::EvalType::kIterSingleBatch, fss::OutputType::kShiftedAdditive);
    }

    /**
     * @brief Get the database size for the MixedOblivSelectParameters.
     * @return uint64_t The input bitsize of DPF parameters.
     */
    uint64_t GetDatabaseSize() const {
        return params_.GetInputBitsize();
    }

    /**
     * @brief Get the DpfParameters for the MixedOblivSelectParameters.
     * @return fss::dpf::DpfParameters The DpfParameters for the MixedOblivSelectParameters.
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
    fss::dpf::DpfParameters params_; /**< DpfParameters for the MixedOblivSelectParameters. */
};

/**
 * @brief A struct representing a key for the Oblivious Selection.
 */
struct MixedOblivSelectKey {
    uint64_t         party_id;      /**< The ID of the party associated with the key. */
    fss::dpf::DpfKey key_from_prev; /**< The DpfKey received from the previous party. */
    fss::dpf::DpfKey key_from_next; /**< The DpfKey received from the next party. */
    uint64_t         rsh_from_prev; /**< The share r received from the previous party. */
    uint64_t         rsh_from_next; /**< The share r received from the next party. */
    uint64_t         wsh_from_prev; /**< The share w received from the previous party. */
    uint64_t         wsh_from_next; /**< The share w received from the next party. */

    /**
     * @brief Default constructor for MixedOblivSelectKey is deleted.
     */
    MixedOblivSelectKey() = delete;

    /**
     * @brief Parameterized constructor for MixedOblivSelectKey.
     * @param id The ID of the party associated with the key.
     * @param params MixedOblivSelectParameters for the MixedOblivSelectKey.
     */
    MixedOblivSelectKey(const uint64_t id, const MixedOblivSelectParameters &params);

    /**
     * @brief Destructor for MixedOblivSelectKey.
     */
    ~MixedOblivSelectKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of MixedOblivSelectKey.
     */
    MixedOblivSelectKey(const MixedOblivSelectKey &)            = delete;
    MixedOblivSelectKey &operator=(const MixedOblivSelectKey &) = delete;

    /**
     * @brief Move constructor for MixedOblivSelectKey.
     */
    MixedOblivSelectKey(MixedOblivSelectKey &&)            = default;
    MixedOblivSelectKey &operator=(MixedOblivSelectKey &&) = default;

    /**
     * @brief Equality operator for MixedOblivSelectKey.
     * @param rhs The MixedOblivSelectKey to compare against.
     * @return True if the MixedOblivSelectKey is equal, false otherwise.
     */
    bool operator==(const MixedOblivSelectKey &rhs) const {
        return (party_id == rhs.party_id) && (key_from_prev == rhs.key_from_prev) && (key_from_next == rhs.key_from_next) &&
               (rsh_from_prev == rhs.rsh_from_prev) && (rsh_from_next == rhs.rsh_from_next) &&
               (wsh_from_prev == rhs.wsh_from_prev) && (wsh_from_next == rhs.wsh_from_next);
    }

    bool operator!=(const MixedOblivSelectKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized MixedOblivSelectKey.
     * @return size_t The size of the serialized MixedOblivSelectKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized MixedOblivSelectKey.
     * @return size_t The size of the serialized MixedOblivSelectKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the MixedOblivSelectKey.
     * @param buffer The buffer to store the serialized MixedOblivSelectKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the MixedOblivSelectKey.
     * @param buffer The buffer to store the serialized MixedOblivSelectKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the MixedOblivSelectKey.
     * @param detailed Flag to print the detailed MixedOblivSelectKey.
     */
    void PrintKey(const bool detailed = false) const;

private:
    MixedOblivSelectParameters params_;          /**< MixedOblivSelectParameters for the MixedOblivSelectKey. */
    size_t                     serialized_size_; /**< The size of the serialized MixedOblivSelectKey. */
};

/**
 * @brief A class to generate keys for the Oblivious Selection.
 */
class MixedOblivSelectKeyGenerator {
public:
    /**
     * @brief Default constructor for MixedOblivSelectKeyGenerator is deleted.
     */
    MixedOblivSelectKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for MixedOblivSelectKeyGenerator.
     * @param params MixedOblivSelectParameters for the MixedOblivSelectKeyGenerator.
     * @param ass Additive sharing for 2-party for the MixedOblivSelectKeyGenerator.
     */
    MixedOblivSelectKeyGenerator(const MixedOblivSelectParameters &params,
                                 sharing::AdditiveSharing2P       &ass);

    /**
     * @brief Offline setup for the MixedOblivSelectKeyGenerator.
     * @param num_selection The number of selections for the MixedOblivSelectKeyGenerator.
     * @param file_path The file path for the MixedOblivSelectKeyGenerator.
     */
    void OfflineSetUp(const uint64_t num_selection, const std::string &file_path) const;

    /**
     * @brief Generate keys for the Oblivious Selection.
     * @return std::array<MixedOblivSelectKey, 3> The array of MixedOblivSelectKey for the Oblivious Selection.
     */
    std::array<MixedOblivSelectKey, 3> GenerateKeys() const;

private:
    MixedOblivSelectParameters  params_; /**< MixedOblivSelectParameters for the MixedOblivSelectKeyGenerator. */
    fss::dpf::DpfKeyGenerator   gen_;    /**< DPF key generator for the MixedOblivSelectKeyGenerator. */
    sharing::AdditiveSharing2P &ass_;    /**< Additive sharing for 2-party for the MixedOblivSelectKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the Oblivious Selection.
 */
class MixedOblivSelectEvaluator {
public:
    /**
     * @brief Default constructor for MixedOblivSelectEvaluator is deleted.
     */
    MixedOblivSelectEvaluator() = delete;

    /**
     * @brief Parameterized constructor for MixedOblivSelectEvaluator.
     * @param params MixedOblivSelectParameters for the MixedOblivSelectEvaluator.
     * @param rss Replicated sharing for 3-party for the MixedOblivSelectEvaluator.
     * @param ass_prev Additive sharing for 2-party for the MixedOblivSelectEvaluator.
     * @param ass_next Additive sharing for 2-party for the MixedOblivSelectEvaluator.
     */
    MixedOblivSelectEvaluator(const MixedOblivSelectParameters &params,
                              sharing::ReplicatedSharing3P     &rss,
                              sharing::AdditiveSharing2P       &ass_prev,
                              sharing::AdditiveSharing2P       &ass_next);

    void OnlineSetUp(const uint64_t party_id, const std::string &file_path) const;

    void Evaluate(Channels                      &chls,
                  const MixedOblivSelectKey     &key,
                  std::vector<block>            &uv_prev,
                  std::vector<block>            &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const MixedOblivSelectKey     &key1,
                           const MixedOblivSelectKey     &key2,
                           std::vector<block>            &uv_prev,
                           std::vector<block>            &uv_next,
                           const sharing::RepShareView64 &database,
                           const sharing::RepShareVec64  &index,
                           sharing::RepShareVec64        &result) const;

    std::pair<uint64_t, uint64_t> EvaluateFullDomainThenDotProduct(
        const uint64_t                 party_id,
        const fss::dpf::DpfKey        &key_prev,
        const fss::dpf::DpfKey        &key_next,
        std::vector<block>            &uv_prev,
        std::vector<block>            &uv_next,
        const sharing::RepShareView64 &database,
        const uint64_t                 pr_prev,
        const uint64_t                 pr_next) const;

private:
    MixedOblivSelectParameters    params_;   /**< MixedOblivSelectParameters for the MixedOblivSelectEvaluator. */
    fss::dpf::DpfEvaluator        eval_;     /**< DPF evaluator for the MixedOblivSelectEvaluator. */
    sharing::ReplicatedSharing3P &rss_;      /**< Replicated sharing for 3-party for the MixedOblivSelectEvaluator. */
    sharing::AdditiveSharing2P   &ass_prev_; /**< Additive sharing for 2-party for the MixedOblivSelectEvaluator. */
    sharing::AdditiveSharing2P   &ass_next_; /**< Additive sharing for 2-party for the MixedOblivSelectEvaluator. */

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructMaskedValue(
        Channels                  &chls,
        const MixedOblivSelectKey &key,
        const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructMaskedValue(
        Channels                     &chls,
        const MixedOblivSelectKey    &key1,
        const MixedOblivSelectKey    &key2,
        const sharing::RepShareVec64 &index) const;
};

}    // namespace proto
}    // namespace fsswm

#endif    // PROTOCOL_MIXED_OBLIV_SELECT_H_
