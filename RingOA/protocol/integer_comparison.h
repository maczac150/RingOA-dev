#ifndef PROTOCOL_INTEGER_COMPARISON_H_
#define PROTOCOL_INTEGER_COMPARISON_H_

#include "ddcf.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace proto {

/**
 * @brief A class to hold parameters for the comparison protocol.
 */
class IntegerComparisonParameters {
public:
    /**
     * @brief Default constructor for IntegerComparisonParameters is deleted.
     */
    IntegerComparisonParameters() = delete;

    /**
     * @brief Parameterized constructor for IntegerComparisonParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    explicit IntegerComparisonParameters(const uint64_t n, const uint64_t e)
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
     * @brief Reconfigure the parameters for the comparison protocol.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    void ReconfigureParameters(const uint64_t n, const uint64_t e) {
        params_.ReconfigureParameters(n, e);
    }

    /**
     * @brief Get the string representation of the IntegerComparisonParameters.
     * @return std::string The string representation of the IntegerComparisonParameters.
     */
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }

    /**
     * @brief Get the DDCF parameters for the comparison protocol.
     * @return const DdcfParameters& The DDCF parameters for the comparison protocol.
     */
    const DdcfParameters &GetParameters() const {
        return params_;
    }

    /**
     * @brief Print the details of the IntegerComparisonParameters.
     */
    void PrintParameters() const;

private:
    DdcfParameters params_; /**< DDCF parameters for the comparison protocol. */
};

/**
 * @brief A class to hold the keys for the comparison protocol.
 */
struct IntegerComparisonKey {
    DdcfKey  ddcf_key; /**< DDCF key for the comparison protocol. */
    uint64_t shr1_in;  /**< Shared random values for the comparison protocol. */
    uint64_t shr2_in;  /**< Shared random values for the comparison protocol. */

    /**
     * @brief Default constructor for IntegerComparisonKey is deleted.
     */
    IntegerComparisonKey() = delete;

    explicit IntegerComparisonKey(const uint64_t party_id, const IntegerComparisonParameters &params);

    /**
     * @brief Destructor for IntegerComparisonKey.
     */
    ~IntegerComparisonKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of IntegerComparisonKey.
     */
    IntegerComparisonKey(const IntegerComparisonKey &)            = delete;
    IntegerComparisonKey &operator=(const IntegerComparisonKey &) = delete;

    /**
     * @brief Move constructor for IntegerComparisonKey.
     */
    IntegerComparisonKey(IntegerComparisonKey &&) noexcept            = default;
    IntegerComparisonKey &operator=(IntegerComparisonKey &&) noexcept = default;

    /**
     * @brief IntegerComparison operator for IntegerComparisonKey.
     * @param rhs The IntegerComparisonKey to compare against.
     * @return True if the IntegerComparisonKey is equal, false otherwise.
     */
    bool operator==(const IntegerComparisonKey &rhs) const {
        return (ddcf_key == rhs.ddcf_key) &&
               (shr1_in == rhs.shr1_in) &&
               (shr2_in == rhs.shr2_in);
    }

    bool operator!=(const IntegerComparisonKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized IntegerComparisonKey.
     * @return size_t The size of the serialized IntegerComparisonKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized IntegerComparisonKey.
     * @return size_t The size of the serialized IntegerComparisonKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the IntegerComparisonKey.
     * @param buffer The buffer to store the serialized IntegerComparisonKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the IntegerComparisonKey.
     * @param buffer The serialized IntegerComparisonKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the details of the IntegerComparisonKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    IntegerComparisonParameters params_;          /**< IntegerComparison parameters for the comparison protocol. */
    size_t                      serialized_size_; /**< The size of the serialized IntegerComparisonKey. */
};

/**
 * @brief A class to generate keys for the comparison protocol.
 */
class IntegerComparisonKeyGenerator {
public:
    /**
     * @brief Default constructor for IntegerComparisonKeyGenerator is deleted.
     */
    IntegerComparisonKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for IntegerComparisonKeyGenerator.
     * @param params IntegerComparisonParameters for the IntegerComparisonKey.
     */
    explicit IntegerComparisonKeyGenerator(
        const IntegerComparisonParameters &params,
        sharing::AdditiveSharing2P        &ss_in,
        sharing::AdditiveSharing2P        &ss_out);

    /**
     * @brief Generate a pair of IntegerComparisonKeys for the comparison protocol.
     * @return std::pair<IntegerComparisonKey, IntegerComparisonKey> A pair of generated IntegerComparisonKeys.
     */
    std::pair<IntegerComparisonKey, IntegerComparisonKey> GenerateKeys() const;

private:
    IntegerComparisonParameters params_; /**< IntegerComparison parameters for the comparison protocol. */
    DdcfKeyGenerator            gen_;    /**< DDCF generator for the comparison protocol. */
    sharing::AdditiveSharing2P &ss_in_;  /**< Additive sharing for inputs. */
    sharing::AdditiveSharing2P &ss_out_; /**< Additive sharing for outputs. */
};

/**
 * @brief A class to evaluate the comparison protocol.
 */
class IntegerComparisonEvaluator {
public:
    /**
     * @brief Default constructor for IntegerComparisonEvaluator is deleted.
     */
    IntegerComparisonEvaluator() = delete;

    /**
     * @brief Parameterized constructor for IntegerComparisonEvaluator.
     * @param params IntegerComparisonParameters for the IntegerComparisonEvaluator.
     */
    explicit IntegerComparisonEvaluator(
        const IntegerComparisonParameters &params,
        sharing::AdditiveSharing2P        &ss_in,
        sharing::AdditiveSharing2P        &ss_out);

    /**
     * @brief Evaluate the integer comparison protocol using the given keys and inputs.
     * @param chl The communication channel for the evaluation.
     * @param key The IntegerComparisonKey to evaluate.
     * @param x1 The first shared input.
     * @param x2 The second shared input.
     * @return uint64_t True if the inputs are equal, false otherwise.
     */
    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const IntegerComparisonKey &key, const uint64_t x1, const uint64_t x2) const;

    /**
     * @brief Evaluate the integer comparison protocol using the given keys and masked inputs.
     * @param key The IntegerComparisonKey to evaluate.
     * @param x1 The first masked input.
     * @param x2 The second masked input.
     * @return uint64_t True if the masked inputs are equal, false otherwise.
     */
    uint64_t EvaluateMaskedInput(const IntegerComparisonKey &key, const uint64_t x1, const uint64_t x2) const;

private:
    IntegerComparisonParameters params_; /**< IntegerComparison parameters for the comparison protocol. */
    DdcfEvaluator               eval_;   /**< DDCF evaluator for the comparison protocol. */
    sharing::AdditiveSharing2P &ss_in_;  /**< Additive sharing for inputs. */
    sharing::AdditiveSharing2P &ss_out_; /**< Additive sharing for outputs. */
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_INTEGER_COMPARISON_H_
