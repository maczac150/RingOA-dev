#ifndef PROTOCOL_SHARED_OT_H_
#define PROTOCOL_SHARED_OT_H_

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

namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace proto {

/**
 * @brief A class to hold params for the Oblivious Selection.
 */
class SharedOtParameters {
public:
    /**
     * @brief Default constructor for SharedOtParameters is deleted.
     */
    SharedOtParameters() = delete;

    /**
     * @brief Parameterized constructor for SharedOtParameters.
     * @param d The input bitsize.
     */
    explicit SharedOtParameters(const uint64_t d, const fss::OutputType mode = fss::OutputType::kShiftedAdditive)
        : params_(d, d, fss::kOptimizedEvalType, mode) {
    }

    /**
     * @brief Reconfigure the parameters for the Zero Test technique.
     * @param d The input bitsize.
     */
    void ReconfigureParameters(const uint64_t d, const fss::OutputType mode = fss::OutputType::kShiftedAdditive) {
        params_.ReconfigureParameters(d, d, fss::kOptimizedEvalType, mode);
    }

    /**
     * @brief Get the database size for the SharedOtParameters.
     * @return uint64_t The input bitsize of DPF parameters.
     */
    uint64_t GetDatabaseSize() const {
        return params_.GetInputBitsize();
    }

    /**
     * @brief Get the DpfParameters for the SharedOtParameters.
     * @return fss::dpf::DpfParameters The DpfParameters for the SharedOtParameters.
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
    fss::dpf::DpfParameters params_; /**< DpfParameters for the SharedOtParameters. */
};

/**
 * @brief A struct representing a key for the Oblivious Selection.
 */
struct SharedOtKey {
    uint64_t         party_id;      /**< The ID of the party associated with the key. */
    fss::dpf::DpfKey key_from_prev; /**< The DpfKey received from the previous party. */
    fss::dpf::DpfKey key_from_next; /**< The DpfKey received from the next party. */
    uint64_t         rsh_from_prev; /**< The share r received from the previous party. */
    uint64_t         rsh_from_next; /**< The share r received from the next party. */

    /**
     * @brief Default constructor for SharedOtKey is deleted.
     */
    SharedOtKey() = delete;

    /**
     * @brief Parameterized constructor for SharedOtKey.
     * @param id The ID of the party associated with the key.
     * @param params SharedOtParameters for the SharedOtKey.
     */
    SharedOtKey(const uint64_t id, const SharedOtParameters &params);

    /**
     * @brief Destructor for SharedOtKey.
     */
    ~SharedOtKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of SharedOtKey.
     */
    SharedOtKey(const SharedOtKey &)            = delete;
    SharedOtKey &operator=(const SharedOtKey &) = delete;

    /**
     * @brief Move constructor for SharedOtKey.
     */
    SharedOtKey(SharedOtKey &&)            = default;
    SharedOtKey &operator=(SharedOtKey &&) = default;

    /**
     * @brief Equality operator for SharedOtKey.
     * @param rhs The SharedOtKey to compare against.
     * @return True if the SharedOtKey is equal, false otherwise.
     */
    bool operator==(const SharedOtKey &rhs) const {
        return (party_id == rhs.party_id) && (key_from_prev == rhs.key_from_prev) && (key_from_next == rhs.key_from_next) &&
               (rsh_from_prev == rhs.rsh_from_prev) && (rsh_from_next == rhs.rsh_from_next);
    }

    bool operator!=(const SharedOtKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized SharedOtKey.
     * @return size_t The size of the serialized SharedOtKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized SharedOtKey.
     * @return size_t The size of the serialized SharedOtKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the SharedOtKey.
     * @param buffer The buffer to store the serialized SharedOtKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the SharedOtKey.
     * @param buffer The buffer to store the serialized SharedOtKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the SharedOtKey.
     * @param detailed Flag to print the detailed SharedOtKey.
     */
    void PrintKey(const bool detailed = false) const;

private:
    SharedOtParameters params_;          /**< SharedOtParameters for the SharedOtKey. */
    size_t             serialized_size_; /**< The size of the serialized SharedOtKey. */
};

/**
 * @brief A class to generate keys for the Oblivious Selection.
 */
class SharedOtKeyGenerator {
public:
    /**
     * @brief Default constructor for SharedOtKeyGenerator is deleted.
     */
    SharedOtKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for SharedOtKeyGenerator.
     * @param params SharedOtParameters for the SharedOtKeyGenerator.
     * @param ass Additive sharing for 2-party for the SharedOtKeyGenerator.
     */
    SharedOtKeyGenerator(const SharedOtParameters   &params,
                         sharing::AdditiveSharing2P &ass);

    /**
     * @brief Generate keys for the Oblivious Selection.
     * @return std::array<SharedOtKey, 3> The array of SharedOtKey for the Oblivious Selection.
     */
    std::array<SharedOtKey, 3> GenerateKeys() const;

private:
    SharedOtParameters          params_; /**< SharedOtParameters for the SharedOtKeyGenerator. */
    fss::dpf::DpfKeyGenerator   gen_;    /**< DPF key generator for the SharedOtKeyGenerator. */
    sharing::AdditiveSharing2P &ass_;    /**< Additive sharing for 2-party for the SharedOtKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the Oblivious Selection.
 */
class SharedOtEvaluator {
public:
    /**
     * @brief Default constructor for SharedOtEvaluator is deleted.
     */
    SharedOtEvaluator() = delete;

    /**
     * @brief Parameterized constructor for SharedOtEvaluator.
     * @param params SharedOtParameters for the SharedOtEvaluator.
     * @param rss Additive replicated sharing for 3-party for the SharedOtEvaluator.
     */
    SharedOtEvaluator(const SharedOtParameters     &params,
                      sharing::ReplicatedSharing3P &rss);

    void Evaluate(Channels                      &chls,
                  const SharedOtKey             &key,
                  std::vector<uint64_t>         &uv_prev,
                  std::vector<uint64_t>         &uv_next,
                  const sharing::RepShareView64 &database,
                  const sharing::RepShare64     &index,
                  sharing::RepShare64           &result) const;

    void Evaluate_Parallel(Channels                      &chls,
                           const SharedOtKey             &key1,
                           const SharedOtKey             &key2,
                           std::vector<uint64_t>         &uv_prev,
                           std::vector<uint64_t>         &uv_next,
                           const sharing::RepShareView64 &database,
                           const sharing::RepShareVec64  &index,
                           sharing::RepShareVec64        &result) const;

    std::pair<uint64_t, uint64_t> EvaluateFullDomainThenDotProduct(
        const uint64_t                 party_id,
        const fss::dpf::DpfKey        &key_from_prev,
        const fss::dpf::DpfKey        &key_from_next,
        std::vector<uint64_t>         &uv_prev,
        std::vector<uint64_t>         &uv_next,
        const sharing::RepShareView64 &database,
        const uint64_t                 pr_prev,
        const uint64_t                 pr_next) const;

private:
    SharedOtParameters               params_; /**< SharedOtParameters for the SharedOtEvaluator. */
    fss::dpf::DpfEvaluator           eval_;   /**< DPF evaluator for the SharedOtEvaluator. */
    sharing::ReplicatedSharing3P    &rss_;    /**< Additive replicated sharing for 3-party for the SharedOtEvaluator. */
    fss::prg::PseudoRandomGenerator &G_;      /**< Pseudo random generator for the SharedOtEvaluator. */

    // Internal functions
    std::pair<uint64_t, uint64_t> ReconstructMaskedValue(
        Channels                  &chls,
        const SharedOtKey         &key,
        const sharing::RepShare64 &index) const;

    std::array<uint64_t, 4> ReconstructMaskedValue(
        Channels                     &chls,
        const SharedOtKey            &key1,
        const SharedOtKey            &key2,
        const sharing::RepShareVec64 &index) const;
};

}    // namespace proto
}    // namespace fsswm

#endif    // PROTOCOL_SHARED_OT_H_
