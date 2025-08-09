#ifndef FSS_DPF_KEY_H_
#define FSS_DPF_KEY_H_

#include <memory>

#include "fss.h"

namespace ringoa {
namespace fss {
namespace dpf {

/**
 * @brief A class to hold params for the Distributed Point Function (DPF).
 */
class DpfParameters {
public:
    /**
     * @brief Default constructor for DpfParameters is deleted.
     */
    DpfParameters() = delete;

    /**
     * @brief Parameterized constructor for DpfParameters.
     * @param n The input bitsize.
     * @param e The element bitsize.
     * @param eval_type The evaluation type for the DPF.
     * @param output_mode The output mode for the DPF.
     */
    explicit DpfParameters(
        const uint64_t n, const uint64_t e,
        EvalType   eval_type   = kOptimizedEvalType,
        OutputType output_mode = OutputType::kShiftedAdditive);

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
     * @brief Get the Early Termination flag.
     * @return bool The flag to enable/disable early termination.
     */
    bool GetEnableEarlyTermination() const {
        return enable_et_;
    }

    /**
     * @brief Get the Terminate Bitsize object
     * @return uint64_t The size of the termination bits.
     */
    uint64_t GetTerminateBitsize() const {
        return terminate_bitsize_;
    }

    /**
     * @brief Get the Evaluation Type.
     * @return EvalType The evaluation type for the DPF.
     */
    EvalType GetEvalType() const {
        return fde_type_;
    }

    /**
     * @brief Get the Output Mode for the DPF.
     * @return OutputType The output mode for the DPF.
     */
    OutputType GetOutputType() const {
        return output_mode_;
    }

    /**
     * @brief Reconfigure the parameters for the DPF.
     * @param n The input bitsize.
     * @param e The element bitsize.
     * @param enable_et Toggle this flag to enable/disable early termination.
     * @param eval_type The evaluation type for the DPF.
     * @param output_mode The output mode for the DPF.
     */
    void ReconfigureParameters(
        const uint64_t n, const uint64_t e,
        EvalType   eval_type   = kOptimizedEvalType,
        OutputType output_mode = OutputType::kShiftedAdditive);

    /**
     * @brief Get the string representation of the DpfParameters.
     * @return std::string The string representation of the DpfParameters.
     */
    std::string GetParametersInfo() const;

    /**
     * @brief Print the details of the DpfParameters.
     */
    void PrintParameters() const;

private:
    uint64_t   input_bitsize_;     /**< The size of input in bits. */
    uint64_t   element_bitsize_;   /**< The size of each element in bits. */
    bool       enable_et_;         /**< Toggle this flag to enable/disable early termination. */
    uint64_t   terminate_bitsize_; /**< The size of the termination bits. */
    EvalType   fde_type_;          /**< The evaluation type for full domain evaluation. */
    OutputType output_mode_;       /**< The output mode for the DPF. */

    void Resolve_();
    void ValidateOrThrow_() const;
};

/**
 * @brief A struct representing a key for the Distributed Point Function (DPF).
 * @warning `DpfKey` must call initialize before use.
 */
struct DpfKey {
    uint64_t                 party_id;         /**< The ID of the party associated with the key. */
    block                    init_seed;        /**< Seed for the DPF key. */
    uint64_t                 cw_length;        /**< Size of Correction words. */
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
    explicit DpfKey(const uint64_t id, const DpfParameters &params);

    /**
     * @brief Destructor for DpfKey.
     */
    ~DpfKey() = default;

    /**
     * @brief Copy constructor is deleted to prevent copying of DpfKey.
     */
    DpfKey(const DpfKey &)            = delete;
    DpfKey &operator=(const DpfKey &) = delete;

    /**
     * @brief Move constructor for DpfKey.
     */
    DpfKey(DpfKey &&) noexcept            = default;
    DpfKey &operator=(DpfKey &&) noexcept = default;

    /**
     * @brief Equality operator for DpfKey.
     * @param rhs The DpfKey to compare against.
     * @return True if the DpfKey is equal, false otherwise.
     */
    bool operator==(const DpfKey &rhs) const {
        bool result = (party_id == rhs.party_id) &&
                      (init_seed == rhs.init_seed) &&
                      (cw_length == rhs.cw_length);
        for (uint64_t i = 0; i < cw_length; ++i) {
            result &= (cw_seed[i] == rhs.cw_seed[i]) &&
                      (cw_control_left[i] == rhs.cw_control_left[i]) &&
                      (cw_control_right[i] == rhs.cw_control_right[i]);
        }
        return result & (output == rhs.output);
    }

    bool operator!=(const DpfKey &rhs) const {
        return !(*this == rhs);
    }

    /**
     * @brief Get the size of the serialized DpfKey.
     * @return size_t The size of the serialized DpfKey.
     */
    size_t GetSerializedSize() const {
        return serialized_size_;
    }

    /**
     * @brief Calculate the size of the serialized DpfKey.
     * @return size_t The size of the serialized DpfKey.
     */
    size_t CalculateSerializedSize() const;

    /**
     * @brief Serialize the DpfKey.
     * @param buffer The buffer to store the serialized DpfKey.
     */
    void Serialize(std::vector<uint8_t> &buffer) const;

    /**
     * @brief Deserialize the DpfKey.
     * @param buffer The serialized DpfKey.
     */
    void Deserialize(const std::vector<uint8_t> &buffer);

    /**
     * @brief Print the details of the DpfKey.
     * @param detailed Flag to print detailed information.
     */
    void PrintKey(const bool detailed = false) const;

private:
    DpfParameters params_;          /**< DpfParameters for the DpfKey. */
    size_t        serialized_size_; /**< The size of the serialized DpfKey. */
};

}    // namespace dpf
}    // namespace fss
}    // namespace ringoa

#endif    // FSS_DPF_KEY_H_
