#ifndef PROTOCOL_ZERO_TEST_H_
#define PROTOCOL_ZERO_TEST_H_

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
 * @brief A class to hold parameters for the zero test protocol.
 */
class ZeroTestParameters {
public:
    /**
     * @brief Default constructor for ZeroTestParameters is deleted.
     */
    ZeroTestParameters() = delete;

    /**
     * @brief Parameterized constructor for ZeroTestParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    explicit ZeroTestParameters(const uint64_t n, const uint64_t e)
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
     * @brief Reconfigure the parameters for the zero test protocol.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    void ReconfigureParameters(const uint64_t n, const uint64_t e) {
        params_.ReconfigureParameters(n, e);
    }

    /**
     * @brief Get the string representation of the ZeroTestParameters.
     * @return std::string The string representation of the ZeroTestParameters.
     */
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }

    /**
     * @brief Get the DPF parameters for the zero test protocol.
     * @return const fss::dpf::DpfParameters& The DPF parameters for the zero test protocol.
     */
    const fss::dpf::DpfParameters &GetParameters() const {
        return params_;
    }

    /**
     * @brief Print the details of the ZeroTestParameters.
     */
    void PrintParameters() const;

private:
    fss::dpf::DpfParameters params_; /**< DPF parameters for the zero test protocol. */
};

/**
 * @brief A class to hold the keys for the zero test protocol.
 */
struct ZeroTestKey {
    fss::dpf::DpfKey dpf_key; /**< DPF key for the zero test protocol. */
    uint64_t         shr_in;  /**< Shared random values for the zero test protocol. */

    /**
     * @brief Default constructor for ZeroTestKey is deleted.
     */
    ZeroTestKey() = delete;

    explicit ZeroTestKey(const uint64_t party_id, const ZeroTestParameters &params);

    /**
     * @brief Destructor for ZeroTestKey.
     */
    ~ZeroTestKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of ZeroTestKey.
     */
    ZeroTestKey(const ZeroTestKey &)            = delete;
    ZeroTestKey &operator=(const ZeroTestKey &) = delete;

    /**
     * @brief Move constructor for ZeroTestKey.
     */
    ZeroTestKey(ZeroTestKey &&) noexcept            = default;
    ZeroTestKey &operator=(ZeroTestKey &&) noexcept = default;

    /**
     * @brief ZeroTest operator for ZeroTestKey.
     * @param rhs The ZeroTestKey to compare against.
     * @return True if the ZeroTestKey is equal, false otherwise.
     */
    bool operator==(const ZeroTestKey &rhs) const {
        return (dpf_key == rhs.dpf_key) &&
               (shr_in == rhs.shr_in);
    }

    bool operator!=(const ZeroTestKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized ZeroTestKey.
     * @return size_t The size of the serialized ZeroTestKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized ZeroTestKey.
     * @return size_t The size of the serialized ZeroTestKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the ZeroTestKey.
     * @param buffer The buffer to store the serialized ZeroTestKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the ZeroTestKey.
     * @param buffer The serialized ZeroTestKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the details of the ZeroTestKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    ZeroTestParameters params_;          /**< ZeroTest parameters for the zero test protocol. */
    size_t             serialized_size_; /**< The size of the serialized ZeroTestKey. */
};

/**
 * @brief A class to generate keys for the zero test protocol.
 */
class ZeroTestKeyGenerator {
public:
    /**
     * @brief Default constructor for ZeroTestKeyGenerator is deleted.
     */
    ZeroTestKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for ZeroTestKeyGenerator.
     * @param params ZeroTestParameters for the ZeroTestKey.
     * @param ss_in AdditiveSharing2P for inputs.
     * @param ss_out AdditiveSharing2P for outputs.
     */
    explicit ZeroTestKeyGenerator(
        const ZeroTestParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    /**
     * @brief Generate a pair of ZeroTestKeys for the zero test protocol.
     * @return std::pair<ZeroTestKey, ZeroTestKey> A pair of generated ZeroTestKeys.
     */
    std::pair<ZeroTestKey, ZeroTestKey> GenerateKeys() const;

private:
    ZeroTestParameters          params_; /**< ZeroTest parameters for the zero test protocol. */
    fss::dpf::DpfKeyGenerator   gen_;    /**< DPF generator for the zero test protocol. */
    sharing::AdditiveSharing2P &ss_in_;  /**< Additive sharing for inputs. */
    sharing::AdditiveSharing2P &ss_out_; /**< Additive sharing for outputs. */
};

/**
 * @brief A class to evaluate the zero test protocol.
 */
class ZeroTestEvaluator {
public:
    /**
     * @brief Default constructor for ZeroTestEvaluator is deleted.
     */
    ZeroTestEvaluator() = delete;

    /**
     * @brief Parameterized constructor for ZeroTestEvaluator.
     * @param params ZeroTestParameters for the ZeroTestEvaluator.
     * @param ss_in AdditiveSharing2P for inputs.
     * @param ss_out AdditiveSharing2P for outputs.
     */
    explicit ZeroTestEvaluator(
        const ZeroTestParameters   &params,
        sharing::AdditiveSharing2P &ss_in,
        sharing::AdditiveSharing2P &ss_out);

    /**
     * @brief Evaluate the zero test protocol using the given keys and inputs.
     * @param chl The communication channel for the evaluation.
     * @param key The ZeroTestKey to evaluate.
     * @param x The shared input.
     * @return uint64_t True if the input is zero, false otherwise.
     */
    uint64_t EvaluateSharedInput(osuCrypto::Channel &chl, const ZeroTestKey &key, const uint64_t x) const;

    /**
     * @brief Evaluate the zero test protocol using the given keys and masked inputs.
     * @param key The ZeroTestKey to evaluate.
     * @param x The  masked input.
     * @return uint64_t True if the masked input is zero, false otherwise.
     */
    uint64_t EvaluateMaskedInput(const ZeroTestKey &key, const uint64_t x) const;

private:
    ZeroTestParameters          params_; /**< ZeroTest parameters for the zero test protocol. */
    fss::dpf::DpfEvaluator      eval_;   /**< DPF evaluator for the zero test protocol. */
    sharing::AdditiveSharing2P &ss_in_;  /**< Additive sharing for inputs. */
    sharing::AdditiveSharing2P &ss_out_; /**< Additive sharing for outputs. */
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_ZERO_TEST_H_
