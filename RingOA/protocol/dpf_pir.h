#ifndef PROTOCOL_DPF_PIR_H_
#define PROTOCOL_DPF_PIR_H_

#include "RingOA/fss/dpf_eval.h"
#include "RingOA/fss/dpf_gen.h"
#include "RingOA/fss/dpf_key.h"
#include "RingOA/sharing/share_types.h"

namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {

namespace sharing {

class AdditiveSharing2P;

}    // namespace sharing

namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace proto {

/**
 * @brief A class to hold params for the DPF PIR.
 */
class DpfPirParameters {
public:
    /**
     * @brief Default constructor for DpfPirParameters is deleted.
     */
    DpfPirParameters() = delete;

    /**
     * @brief Parameterized constructor for DpfPirParameters.
     * @param d The input bitsize.
     */
    explicit DpfPirParameters(const uint64_t d, const fss::OutputType mode = fss::OutputType::kShiftedAdditive)
        : params_(d, 1, fss::kOptimizedEvalType, mode) {
    }

    /**
     * @brief Reconfigure the parameters for the Zero Test technique.
     * @param d The input bitsize.
     */
    void ReconfigureParameters(const uint64_t d, const fss::OutputType mode = fss::OutputType::kShiftedAdditive) {
        params_.ReconfigureParameters(d, 1, fss::kOptimizedEvalType, mode);
    }

    /**
     * @brief Get the database size for the DpfPirParameters.
     * @return uint64_t The input bitsize of DPF parameters.
     */
    uint64_t GetDatabaseSize() const {
        return params_.GetInputBitsize();
    }

    /**
     * @brief Get the DpfParameters for the DpfPirParameters.
     * @return fss::dpf::DpfParameters The DpfParameters for the DpfPirParameters.
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
    fss::dpf::DpfParameters params_; /**< DpfParameters for the DpfPirParameters. */
};

/**
 * @brief A struct representing a key for the DPF PIR.
 */
struct DpfPirKey {
    fss::dpf::DpfKey dpf_key;
    uint64_t         shr_in;

    /**
     * @brief Default constructor for DpfPirKey is deleted.
     */
    DpfPirKey() = delete;

    /**
     * @brief Parameterized constructor for DpfPirKey.
     * @param id The ID of the party associated with the key.
     * @param params DpfPirParameters for the DpfPirKey.
     */
    DpfPirKey(const uint64_t id, const DpfPirParameters &params);

    /**
     * @brief Destructor for DpfPirKey.
     */
    ~DpfPirKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of DpfPirKey.
     */
    DpfPirKey(const DpfPirKey &)            = delete;
    DpfPirKey &operator=(const DpfPirKey &) = delete;

    /**
     * @brief Move constructor for DpfPirKey.
     */
    DpfPirKey(DpfPirKey &&)            = default;
    DpfPirKey &operator=(DpfPirKey &&) = default;

    /**
     * @brief Equality operator for DpfPirKey.
     * @param rhs The DpfPirKey to compare against.
     * @return True if the DpfPirKey is equal, false otherwise.
     */
    bool operator==(const DpfPirKey &rhs) const {
        return (dpf_key == rhs.dpf_key) && (shr_in == rhs.shr_in);
    }

    bool operator!=(const DpfPirKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized DpfPirKey.
     * @return size_t The size of the serialized DpfPirKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized DpfPirKey.
     * @return size_t The size of the serialized DpfPirKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the DpfPirKey.
     * @param buffer The buffer to store the serialized DpfPirKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the DpfPirKey.
     * @param buffer The buffer to store the serialized DpfPirKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the DpfPirKey.
     * @param detailed Flag to print the detailed DpfPirKey.
     */
    void PrintKey(const bool detailed = false) const;

private:
    DpfPirParameters params_;          /**< DpfPirParameters for the DpfPirKey. */
    size_t           serialized_size_; /**< The size of the serialized DpfPirKey. */
};

/**
 * @brief A class to generate keys for the DPF PIR.
 */
class DpfPirKeyGenerator {
public:
    /**
     * @brief Default constructor for DpfPirKeyGenerator is deleted.
     */
    DpfPirKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for DpfPirKeyGenerator.
     * @param params DpfPirParameters for the DpfPirKeyGenerator.
     * @param ss Sharing scheme for the DpfPirKeyGenerator.
     */
    DpfPirKeyGenerator(const DpfPirParameters     &params,
                       sharing::AdditiveSharing2P &ss);

    /**
     * @brief Generate keys for the DPF PIR.
     * @return std::pair<DpfPirKey, DpfPirKey> The array of DpfPirKey for the DPF PIR.
     */
    std::pair<DpfPirKey, DpfPirKey> GenerateKeys() const;

private:
    DpfPirParameters            params_; /**< DpfPirParameters for the DpfPirKeyGenerator. */
    fss::dpf::DpfKeyGenerator   gen_;    /**< DPF key generator for the DpfPirKeyGenerator. */
    sharing::AdditiveSharing2P &ss_;     /**< Sharing scheme for the DpfPirKeyGenerator. */
};

/**
 * @brief A class to evaluate keys for the DPF PIR.
 */
class DpfPirEvaluator {
public:
    /**
     * @brief Default constructor for DpfPirEvaluator is deleted.
     */
    DpfPirEvaluator() = delete;

    /**
     * @brief Parameterized constructor for DpfPirEvaluator.
     * @param params DpfPirParameters for the DpfPirEvaluator.
     * @param ss Sharing scheme for the DpfPirEvaluator.
     */
    DpfPirEvaluator(const DpfPirParameters &params, sharing::AdditiveSharing2P &ss);

    uint64_t EvaluateSharedInput(osuCrypto::Channel          &chl,
                                 const DpfPirKey             &key,
                                 const std::vector<uint64_t> &database,
                                 const uint64_t               index) const;

    uint64_t EvaluateMaskedInput(const DpfPirKey             &key,
                                 const std::vector<uint64_t> &database,
                                 const uint64_t               index) const;

    block    ComputeDotProductBlockSIMD(const fss::dpf::DpfKey &key, std::vector<block> &database) const;
    uint64_t EvaluateFullDomainThenDotProduct(const fss::dpf::DpfKey &key, const std::vector<uint64_t> &database, std::vector<block> &outputs) const;

private:
    DpfPirParameters                 params_; /**< DpfPirParameters for the DpfPirEvaluator. */
    fss::dpf::DpfEvaluator           eval_;   /**< DPF evaluator for the DpfPirEvaluator. */
    sharing::AdditiveSharing2P      &ss_;     /**< Sharing scheme for the DpfPirEvaluator. */
    fss::prg::PseudoRandomGenerator &G_;      /**< Pseudo random generator for the DpfPirEvaluator. */

    void EvaluateNextSeed(const uint64_t current_level, const block &current_seed, const bool &current_control_bit,
                          std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
                          const fss::dpf::DpfKey &key) const;
};

}    // namespace proto
}    // namespace ringoa

#endif    // PROTOCOL_DPF_PIR_H_
