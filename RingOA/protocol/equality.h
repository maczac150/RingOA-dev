#ifndef PROTOCOL_EQUALITY_H_
#define PROTOCOL_EQUALITY_H_

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

/**
 * @brief A class to hold parameters for the equality protocol.
 */
class EqualityParameters {
public:
    /**
     * @brief Default constructor for EqualityParameters is deleted.
     */
    EqualityParameters() = delete;

    /**
     * @brief Parameterized constructor for EqualityParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    explicit EqualityParameters(const uint64_t n, const uint64_t e)
        : params_(n, e) {
    }

    /**
     * @brief Get the Input Bitsize object
     * @return uint64_t The size of input in bits.
     */
    uint64_t GetInputBitsize() const {
        return params_.GetInputBitsize();
    }

    /**
     * @brief Get the Element Bitsize object
     * @return uint64_t The size of each element in bits.
     */
    uint64_t GetOutputBitsize() const {
        return params_.GetOutputBitsize();
    }

    /**
     * @brief Reconfigure the parameters for the equality protocol.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    void ReconfigureParameters(const uint64_t n, const uint64_t e) {
        params_.ReconfigureParameters(n, e);
    }

    /**
     * @brief Get the string representation of the EqualityParameters.
     * @return std::string The string representation of the EqualityParameters.
     */
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }

    /**
     * @brief Get the DPF parameters for the equality protocol.
     * @return const fss::dpf::DpfParameters& The DPF parameters for the equality protocol.
     */
    const fss::dpf::DpfParameters &GetParameters() const {
        return params_;
    }

    /**
     * @brief Print the details of the EqualityParameters.
     */
    void PrintParameters() const;

private:
    fss::dpf::DpfParameters params_; /**< DPF parameters for the equality protocol. */
};

/**
 * @brief A class to hold the keys for the equality protocol.
 */
struct EqualityKey {
    fss::dpf::DpfKey dpf_key; /**< DPF key for the equality protocol. */
    uint64_t         shr1_in; /**< Shared random values for the equality protocol. */
    uint64_t         shr2_in; /**< Shared random values for the equality protocol. */

    /**
     * @brief Default constructor for EqualityKey is deleted.
     */
    EqualityKey() = delete;

    explicit EqualityKey(const uint64_t party_id, const EqualityParameters &params);

    /**
     * @brief Destructor for EqualityKey.
     */
    ~EqualityKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of EqualityKey.
     */
    EqualityKey(const EqualityKey &)            = delete;
    EqualityKey &operator=(const EqualityKey &) = delete;

    /**
     * @brief Move constructor for EqualityKey.
     */
    EqualityKey(EqualityKey &&) noexcept            = default;
    EqualityKey &operator=(EqualityKey &&) noexcept = default;

    /**
     * @brief Equality operator for EqualityKey.
     * @param rhs The EqualityKey to compare against.
     * @return True if the EqualityKey is equal, false otherwise.
     */
    bool operator==(const EqualityKey &rhs) const {
        return (dpf_key == rhs.dpf_key) &&
               (shr1_in == rhs.shr1_in) &&
               (shr2_in == rhs.shr2_in);
    }

    bool operator!=(const EqualityKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized EqualityKey.
     * @return size_t The size of the serialized EqualityKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized EqualityKey.
     * @return size_t The size of the serialized EqualityKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the EqualityKey.
     * @param buffer The buffer to store the serialized EqualityKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the EqualityKey.
     * @param buffer The serialized EqualityKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the details of the EqualityKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    EqualityParameters params_;          /**< Equality parameters for the equality protocol. */
    size_t             serialized_size_; /**< The size of the serialized EqualityKey. */
};

/**
 * @brief A class to generate keys for the equality protocol.
 */
class EqualityKeyGenerator {
public:
    /**
     * @brief Default constructor for EqualityKeyGenerator is deleted.
     */
    EqualityKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for EqualityKeyGenerator.
     * @param params EqualityParameters for the EqualityKey.
     * @param ss_in AdditiveSharing2P for inputs.
     * @param ss_out AdditiveSharing2P for outputs.
     */
    explicit EqualityKeyGenerator(
        const EqualityParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    /**
     * @brief Generate a pair of EqualityKeys for the equality protocol.
     * @return std::pair<EqualityKey, EqualityKey> A pair of generated EqualityKeys.
     */
    std::pair<EqualityKey, EqualityKey> GenerateKeys() const;

private:
    EqualityParameters          params_; /**< Equality parameters for the equality protocol. */
    fss::dpf::DpfKeyGenerator   gen_;    /**< DPF generator for the equality protocol. */
    sharing::AdditiveSharing2P &ss_in_;  /**< Additive sharing for inputs. */
    sharing::AdditiveSharing2P &ss_out_; /**< Additive sharing for outputs. */
};

/**
 * @brief A class to evaluate the equality protocol.
 * Input: Two shared inputs x1 and x2.
 * Output: A shared output y, where y = 1 if x1 == x2, otherwise y = 0.
 */
class EqualityEvaluator {
public:
    /**
     * @brief Default constructor for EqualityEvaluator is deleted.
     */
    EqualityEvaluator() = delete;

    /**
     * @brief Parameterized constructor for EqualityEvaluator.
     * @param params EqualityParameters for the EqualityEvaluator.
     * @param ss_in AdditiveSharing2P for inputs.
     * @param ss_out AdditiveSharing2P for outputs.
     */
    explicit EqualityEvaluator(
        const EqualityParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    /**
     * @brief Evaluate the equality protocol using the given keys and inputs.
     * @param chl The communication channel for the evaluation.
     * @param key The EqualityKey to evaluate.
     * @param x1 The first shared input.
     * @param x2 The second shared input.
     * @return uint64_t True if the inputs are equal, false otherwise.
     */
    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const EqualityKey &key, const uint64_t x1, const uint64_t x2) const;

    /**
     * @brief Evaluate the equality protocol using the given keys and masked inputs.
     * @param key The EqualityKey to evaluate.
     * @param x1 The first masked input.
     * @param x2 The second masked input.
     * @return uint64_t True if the masked inputs are equal, false otherwise.
     */
    uint64_t EvaluateMaskedInput(const EqualityKey &key, const uint64_t x1, const uint64_t x2) const;

private:
    EqualityParameters          params_; /**< Equality parameters for the equality protocol. */
    fss::dpf::DpfEvaluator      eval_;   /**< DPF evaluator for the equality protocol. */
    sharing::AdditiveSharing2P &ss_in_;  /**< Additive sharing for inputs. */
    sharing::AdditiveSharing2P &ss_out_; /**< Additive sharing for outputs. */
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_EQUALITY_H_
