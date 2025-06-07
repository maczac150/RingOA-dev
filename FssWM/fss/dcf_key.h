#ifndef FSS_DCF_KEY_H_
#define FSS_DCF_KEY_H_

#include <memory>

#include "fss.h"

namespace fsswm {
namespace fss {
namespace dcf {

/**
 * @brief A class to hold params for the Distributed Comparison Function (DCF).
 */
class DcfParameters {
public:
    /**
     * @brief Default constructor for DcfParameters is deleted.
     */
    DcfParameters() = delete;

    /**
     * @brief Parameterized constructor for DcfParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    explicit DcfParameters(const uint64_t n, const uint64_t e);

    /**
     * @brief Get the Input Bitsize object
     * @return uint64_t The size of input in bits.
     */
    uint64_t GetInputBitsize() const {
        return input_bitsize_;
    }

    /**
     * @brief Get the Element Bitsize object
     * @return uint64_t The size of each element in bits.
     */
    uint64_t GetOutputBitsize() const {
        return element_bitsize_;
    }

    /**
     * @brief Validate the parameters for the DCF.
     * @return True if the parameters are valid, false otherwise.
     */
    bool ValidateParameters() const;

    /**
     * @brief Reconfigure the parameters for the DCF.
     * @param n The input bitsize.
     * @param e The element bitsize.
     */
    void ReconfigureParameters(const uint64_t n, const uint64_t e);

    /**
     * @brief Get the string representation of the DcfParameters.
     * @return std::string The string representation of the DcfParameters.
     */
    std::string GetParametersInfo() const;

    /**
     * @brief Print the details of the DcfParameters.
     */
    void PrintParameters() const;

private:
    uint64_t input_bitsize_;   /**< The size of input in bits. */
    uint64_t element_bitsize_; /**< The size of each element in bits. */
};

/**
 * @brief A struct representing a key for the Distributed Comparison Function (DCF).
 * @warning `DcfKey` must call initialize before use.
 */
struct DcfKey {
    uint64_t                    party_id;         /**< The ID of the party associated with the key. */
    block                       init_seed;        /**< Seed for the DCF key. */
    uint64_t                    cw_length;        /**< Size of Correction words. */
    std::unique_ptr<block[]>    cw_seed;          /**< Correction words for the seed. */
    std::unique_ptr<bool[]>     cw_control_left;  /**< Correction words for the left control bits. */
    std::unique_ptr<bool[]>     cw_control_right; /**< Correction words for the right control bits. */
    std::unique_ptr<uint64_t[]> cw_value;         /**< Correction words for the value. */
    uint64_t                    output;           /**< Output of the DCF key. */

    /**
     * @brief Default constructor for DcfKey is deleted.
     */
    DcfKey() = delete;

    /**
     * @brief Parameterized constructor for DcfKey.
     * @param id The ID of the party associated with the key.
     * @param params DcfParameters for the DcfKey.
     */
    explicit DcfKey(const uint64_t id, const DcfParameters &params);

    /**
     * @brief Destructor for DcfKey.
     */
    ~DcfKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of DcfKey.
     */
    DcfKey(const DcfKey &)            = delete;
    DcfKey &operator=(const DcfKey &) = delete;

    /**
     * @brief Move constructor for DcfKey.
     */
    DcfKey(DcfKey &&) noexcept            = default;
    DcfKey &operator=(DcfKey &&) noexcept = default;

    /**
     * @brief Equality operator for DcfKey.
     * @param rhs The DcfKey to compare against.
     * @return True if the DcfKey is equal, false otherwise.
     */
    bool operator==(const DcfKey &rhs) const {
        bool result = (party_id == rhs.party_id) &&
                      (init_seed == rhs.init_seed) &&
                      (cw_length == rhs.cw_length);
        for (uint64_t i = 0; i < cw_length; ++i) {
            result &= (cw_seed[i] == rhs.cw_seed[i]) &&
                      (cw_control_left[i] == rhs.cw_control_left[i]) &&
                      (cw_control_right[i] == rhs.cw_control_right[i]) &&
                      (cw_value[i] == rhs.cw_value[i]);
        }
        return result & (output == rhs.output);
    }

    bool operator!=(const DcfKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized DcfKey.
     * @return size_t The size of the serialized DcfKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized DcfKey.
     * @return size_t The size of the serialized DcfKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the DcfKey.
     * @param buffer The buffer to store the serialized DcfKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the DcfKey.
     * @param buffer The serialized DcfKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the details of the DcfKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    DcfParameters params_;          /**< DcfParameters for the DcfKey. */
    size_t        serialized_size_; /**< The size of the serialized DcfKey. */
};

}    // namespace dcf
}    // namespace fss
}    // namespace fsswm

#endif    // FSS_DCF_KEY_H_
