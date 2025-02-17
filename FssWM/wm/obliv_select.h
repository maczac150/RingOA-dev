#ifndef WM_OBLIV_SELECT_H_
#define WM_OBLIV_SELECT_H_

#include <array>
#include <cstdint>
#include <vector>

#include "FssWM/fss/dpf_eval.h"
#include "FssWM/fss/dpf_gen.h"
#include "FssWM/fss/dpf_key.h"

namespace fsswm {

namespace sharing {

class ReplicatedSharing3P;
class AdditiveSharing2P;
struct Channels;

}    // namespace sharing

namespace wm {

/**
 * @brief A class to hold params for the Oblivious Selection.
 */
struct OblivSelectParameters {
public:
    /**
     * @brief Default constructor for OblivSelectParameters is deleted.
     */
    OblivSelectParameters() = delete;

    /**
     * @brief Parameterized constructor for OblivSelectParameters.
     * @param d The input bitsize.
     */
    explicit OblivSelectParameters(const uint32_t d)
        : params_(d, d) {
    }

    /**
     * @brief Reconfigure the parameters for the Zero Test technique.
     * @param d The input bitsize.
     */
    void ReconfigureParameters(const uint32_t d) {
        params_.ReconfigureParameters(d, d);
    }

    /**
     * @brief Get the DpfParameters for the OblivSelectParameters.
     * @return fss::dpf::DpfParameters The DpfParameters for the OblivSelectParameters.
     */
    fss::dpf::DpfParameters GetParameters() const {
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
    fss::dpf::DpfParameters params_; /**< DpfParameters for the OblivSelectParameters. */
};

/**
 * @brief A struct representing a key for the Oblivious Selection.
 */
struct OblivSelectKey {
    fss::dpf::DpfKey key0;   /**< The DpfKey for the key 0. */
    fss::dpf::DpfKey key1;   /**< The DpfKey for the key 1. */
    uint32_t         shr_r0; /**< The share r0 for the key 0. */
    uint32_t         shr_r1; /**< The share r1 for the key 1. */

    /**
     * @brief Default constructor for OblivSelectKey is deleted.
     */
    OblivSelectKey() = delete;

    /**
     * @brief Parameterized constructor for OblivSelectKey.
     * @param id The ID of the party associated with the key.
     * @param params OblivSelectParameters for the OblivSelectKey.
     */
    OblivSelectKey(const uint32_t id, const OblivSelectParameters &params);

    /**
     * @brief Destructor for OblivSelectKey.
     */
    ~OblivSelectKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of OblivSelectKey.
     */
    OblivSelectKey(const OblivSelectKey &)            = delete;
    OblivSelectKey &operator=(const OblivSelectKey &) = delete;

    /**
     * @brief Move constructor for OblivSelectKey.
     */
    OblivSelectKey(OblivSelectKey &&)            = default;
    OblivSelectKey &operator=(OblivSelectKey &&) = default;

    /**
     * @brief Equality operator for OblivSelectKey.
     * @param rhs The OblivSelectKey to compare against.
     * @return True if the OblivSelectKey is equal, false otherwise.
     */
    bool operator==(const OblivSelectKey &rhs) const {
        return (key0 == rhs.key0) && (key1 == rhs.key1) && (shr_r0 == rhs.shr_r0) && (shr_r1 == rhs.shr_r1);
    }

    bool operator!=(const OblivSelectKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized OblivSelectKey.
     * @return size_t The size of the serialized OblivSelectKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized OblivSelectKey.
     * @return size_t The size of the serialized OblivSelectKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the OblivSelectKey.
     * @param buffer The buffer to store the serialized OblivSelectKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the OblivSelectKey.
     * @param buffer The buffer to store the serialized OblivSelectKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the OblivSelectKey.
     * @param detailed Flag to print the detailed OblivSelectKey.
     */
    void PrintKey(const bool detailed = false) const;

private:
    OblivSelectParameters params_;          /**< OblivSelectParameters for the OblivSelectKey. */
    size_t                serialized_size_; /**< The size of the serialized OblivSelectKey. */
};

/**
 * @brief A class to generate keys for the Oblivious Selection.
 */
class OblivSelectKeyGenerator {
public:
    /**
     * @brief Default constructor for OblivSelectKeyGenerator is deleted.
     */
    OblivSelectKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for OblivSelectKeyGenerator.
     * @param params OblivSelectParameters for the OblivSelectKeyGenerator.
     * @param rss Replicated sharing for 3-party for the OblivSelectKeyGenerator.
     */
    OblivSelectKeyGenerator(const OblivSelectParameters &params, sharing::ReplicatedSharing3P &rss, sharing::AdditiveSharing2P &ass);

    /**
     * @brief Generate keys for the Oblivious Selection.
     * @return std::array<OblivSelectKey, 3> The array of OblivSelectKey for the Oblivious Selection.
     */
    std::array<OblivSelectKey, 3> GenerateKeys() const;

private:
    OblivSelectParameters         params_; /**< OblivSelectParameters for the OblivSelectKeyGenerator. */
    fss::dpf::DpfKeyGenerator     gen_;    /**< DPF key generator for the OblivSelectKeyGenerator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the OblivSelectKeyGenerator. */
    sharing::AdditiveSharing2P   &ass_;    /**< Additive sharing for 2-party for the OblivSelectKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the Oblivious Selection.
 */
class OblivSelectEvaluator {
public:
    /**
     * @brief Default constructor for OblivSelectEvaluator is deleted.
     */
    OblivSelectEvaluator() = delete;

    /**
     * @brief Parameterized constructor for OblivSelectEvaluator.
     * @param params OblivSelectParameters for the OblivSelectEvaluator.
     * @param rss Replicated sharing for 3-party for the OblivSelectEvaluator.
     */
    OblivSelectEvaluator(const OblivSelectParameters &params, sharing::ReplicatedSharing3P &rss);

    uint32_t Evaluate(sharing::Channels &chls, const OblivSelectKey &key, const uint32_t index) const;

private:
    OblivSelectParameters         params_; /**< OblivSelectParameters for the OblivSelectEvaluator. */
    fss::dpf::DpfEvaluator        eval_;   /**< DPF evaluator for the OblivSelectEvaluator. */
    sharing::ReplicatedSharing3P &rss_;    /**< Replicated sharing for 3-party for the OblivSelectEvaluator. */
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_OBLIV_SELECT_H_
