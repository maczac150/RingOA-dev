#ifndef FSS_DPF_KEY_H_
#define FSS_DPF_KEY_H_

#include "fss.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

/**
 * @struct DpfParameters
 * @brief A struct to hold params for the Distributed Point Function (DPF).
 */
struct DpfParameters {
    const uint32_t input_bitsize;            /**< The size of input in bits. */
    const uint32_t element_bitsize;          /**< The size of each element in bits. */
    bool           enable_early_termination; /**< Toggle this flag to enable/disable early termination. */
    const uint32_t terminate_bitsize;        /**< The size of the termination bits. */

    /**
     * @brief Default constructor for DpfParameters is deleted.
     */
    DpfParameters() = delete;

    /**
     * @brief Parameterized constructor for DpfParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     * @param enable_et Toggle this flag to enable/disable early termination.
     */
    DpfParameters(const uint32_t n, const uint32_t e, const bool enable_et = true);

    /**
     * @brief Compute the number of levels to terminate the DPF evaluation.
     * @return The number of levels to terminate the DPF evaluation.
     */
    uint32_t ComputeTerminateLevel();

    /**
     * @brief Validate the parameters for the DPF.
     * @return True if the parameters are valid, false otherwise.
     */
    bool ValidateParameters() const;

    /**
     * @brief Print the details of the DpfParameters.
     */
    void PrintDpfParameters() const;
};

/**
 * @struct DpfKey
 * @brief A struct representing a key for the Distributed Point Function (DPF).
 * @warning `DpfKey` must call initialize before use.
 */
struct DpfKey {
    uint32_t                 party_id;         /**< The ID of the party associated with the key. */
    block                    init_seed;        /**< Seed for the DPF key. */
    uint32_t                 cw_length;        /**< Size of Correction words. */
    std::unique_ptr<block[]> cw_seed;          /**< Correction words for the seed. */
    std::unique_ptr<bool[]>  cw_control_left;  /**< Correction words for the left control bits. */
    std::unique_ptr<bool[]>  cw_control_right; /**< Correction words for the right control bits. */
    block                    output;           /**< Output of the DPF key. */

    /**
     * @brief Default constructor for DpfKey is deleted.
     */
    DpfKey() = delete;

    /**
     * @brief Parameterized constructor for DpfKey.
     * @param id The ID of the party associated with the key.
     * @param params DpfParameters for the DpfKey.
     */
    DpfKey(const uint32_t id, const DpfParameters &params);

    /**
     * @brief Destructor for DpfKey.
     */
    ~DpfKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of DpfKey.
     */
    DpfKey(const DpfKey &) = delete;

    /**
     * @brief Copy assignment operator is deleted to prevent copying of DpfKey.
     */
    DpfKey &operator=(const DpfKey &) = delete;

    /**
     * @brief Move constructor for DpfKey.
     */
    DpfKey(DpfKey &&) noexcept = default;

    /**
     * @brief Move assignment operator for DpfKey.
     */
    DpfKey &operator=(DpfKey &&) noexcept = default;

    /**
     * @brief Equality operator for DpfKey.
     * @param rhs The DpfKey to compare against.
     * @return True if the DpfKey is equal, false otherwise.
     */
    bool operator==(const DpfKey &rhs) const {
        bool result = (party_id == rhs.party_id) && (fss::Equal(init_seed, rhs.init_seed)) && (cw_length == rhs.cw_length);
        for (uint32_t i = 0; i < cw_length; i++) {
            result &= (fss::Equal(cw_seed[i], rhs.cw_seed[i])) && (cw_control_left[i] == rhs.cw_control_left[i]) && (cw_control_right[i] == rhs.cw_control_right[i]);
        }
        return result & (fss::Equal(output, rhs.output));
    }

    bool operator!=(const DpfKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Print the details of the DpfKey.
     * @param n The size of input in bits.
     * @param params DpfParameters for the DpfKey.
     * @param debug Toggle this flag to enable/disable debugging.
     */
    void PrintDpfKey(const bool debug) const;

private:
    DpfParameters params_;
};

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm

#endif    // FSS_DPF_KEY_H_
