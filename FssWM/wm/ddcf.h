#ifndef WM_DDCF_KEY_H_
#define WM_DDCF_KEY_H_

#include "FssWM/fss/dcf_eval.h"
#include "FssWM/fss/dcf_gen.h"
#include "FssWM/fss/dcf_key.h"

namespace fsswm {

namespace sharing {

class BinarySharing2P;

}    // namespace sharing

namespace fss {
namespace prg {

class PseudoRandomGenerator;

}    // namespace prg
}    // namespace fss

namespace wm {

/**
 * @brief A class to hold params for the Distributed Comparison Function (DDCF).
 */
class DdcfParameters {
public:
    /**
     * @brief Default constructor for DdcfParameters is deleted.
     */
    DdcfParameters() = delete;

    /**
     * @brief Parameterized constructor for DdcfParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    explicit DdcfParameters(const uint64_t n, const uint64_t e)
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
     * @brief Reconfigure the parameters for the DDCF.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    void ReconfigureParameters(const uint64_t n, const uint64_t e) {
        params_.ReconfigureParameters(n, e);
    }

    /**
     * @brief Get the DcfParameters for the DdcfParameters.
     * @return dcf::DcfParameters The DcfParameters for the DdcfParameters.
     */
    const fss::dcf::DcfParameters GetParameters() const {
        return params_;
    }

    /**
     * @brief Get the string representation of the DdcfParameters.
     * @return std::string The string representation of the DdcfParameters.
     */
    std::string GetParametersInfo() const {
        return params_.GetParametersInfo();
    }

    /**
     * @brief Print the details of the DdcfParameters.
     */
    void PrintParameters() const;

private:
    fss::dcf::DcfParameters params_; /**< DCF parameters for the DDCF. */
};

/**
 * @brief A struct representing a key for the Distributed Comparison Function (DDCF).
 * @warning `DdcfKey` must call initialize before use.
 */
struct DdcfKey {
    fss::dcf::DcfKey dcf_key;
    uint64_t         mask;

    /**
     * @brief Default constructor for DdcfKey is deleted.
     */
    DdcfKey() = delete;

    /**
     * @brief Parameterized constructor for DdcfKey.
     * @param id The ID of the party associated with the key.
     * @param params DdcfParameters for the DdcfKey.
     */
    explicit DdcfKey(const uint64_t id, const DdcfParameters &params);

    /**
     * @brief Destructor for DdcfKey.
     */
    ~DdcfKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of DdcfKey.
     */
    DdcfKey(const DdcfKey &)            = delete;
    DdcfKey &operator=(const DdcfKey &) = delete;

    /**
     * @brief Move constructor for DdcfKey.
     */
    DdcfKey(DdcfKey &&) noexcept            = default;
    DdcfKey &operator=(DdcfKey &&) noexcept = default;

    /**
     * @brief Equality operator for DdcfKey.
     * @param rhs The DdcfKey to compare against.
     * @return True if the DdcfKey is equal, false otherwise.
     */
    bool operator==(const DdcfKey &rhs) const {
        return dcf_key == rhs.dcf_key &&
               mask == rhs.mask;
    }

    bool operator!=(const DdcfKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized DdcfKey.
     * @return size_t The size of the serialized DdcfKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized DdcfKey.
     * @return size_t The size of the serialized DdcfKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the DdcfKey.
     * @param buffer The buffer to store the serialized DdcfKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the DdcfKey.
     * @param buffer The serialized DdcfKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the details of the DdcfKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    DdcfParameters params_;          /**< DdcfParameters for the DdcfKey. */
    size_t         serialized_size_; /**< The size of the serialized DdcfKey. */
};

/**
 * @brief A class to generate a Distributed Point Function (DDCF) key.
 */
class DdcfKeyGenerator {
public:
    /**
     * @brief Default constructor for DdcfKeyGenerator.
     */
    DdcfKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for DdcfKeyGenerator.
     * @param params DdcfParameters for the DdcfKey.
     */
    explicit DdcfKeyGenerator(const DdcfParameters &params, sharing::BinarySharing2P &bss);

    /**
     * @brief Generate a DDCF key for the given alpha and beta values.
     * @param alpha The alpha value for the DDCF key.
     * @param beta_1 The first beta value for the DDCF key.
     * @param beta_2 The second beta value for the DDCF key.
     * @return A pair of DdcfKey for the DDCF key.
     */
    std::pair<DdcfKey, DdcfKey> GenerateKeys(uint64_t alpha, uint64_t beta_1, uint64_t beta_2) const;

private:
    DdcfParameters            params_; /**< DDCF parameters for the DDCF key. */
    fss::dcf::DcfKeyGenerator gen_;    /**< DCF key generator for the DDCF key. */
    sharing::BinarySharing2P &bss_;    /**< Binary sharing for 2-party for the DDCF key. */
};

/**
 * @brief A class to evaluate DDCF keys.
 */
class DdcfEvaluator {
public:
    /**
     * @brief Default constructor for DdcfEvaluator.
     */
    DdcfEvaluator() = delete;

    /**
     * @brief Parameterized constructor for DdcfEvaluator.
     * @param params DdcfParameters for the DdcfKey.
     */
    explicit DdcfEvaluator(const DdcfParameters &params, sharing::BinarySharing2P &bss);

    /**
     * @brief Evaluate the DDCF key at the given x value.
     * @param key The DDCF key to evaluate.
     * @param x The x value to evaluate the DDCF key.
     * @return The evaluated value at x.
     */
    uint64_t EvaluateAt(const DdcfKey &key, uint64_t x) const;

private:
    DdcfParameters            params_; /**< DDCF parameters for the DDCF key. */
    fss::dcf::DcfEvaluator    eval_;   /**< DCF evaluator for the DDCF key. */
    sharing::BinarySharing2P &bss_;    /**< Binary sharing for 2-party for the DDCF key. */
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_DDCF_KEY_H_
