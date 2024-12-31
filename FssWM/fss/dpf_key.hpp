#ifndef FSS_DPF_KEY_H_
#define FSS_DPF_KEY_H_

#include "fss.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

/**
 * @enum EvalType
 * @brief Enumeration for the evaluation types for DPF.
 */
enum class EvalType
{
    // Evaluation types for DPF
    kNaive,
    kRecursion,
    kIterSingle,
    kIterDouble,
    kIterSingleBatch,
    kIterDoubleBatch,    // ! This is working but too slow
};

const EvalType kDefaultEvalType   = EvalType::kIterSingle;
const EvalType kOptimizedEvalType = EvalType::kIterSingleBatch;

/**
 * @brief Get the string representation of the evaluation type.
 * @param eval_type The evaluation type for the DPF key.
 * @return std::string The string representation of the evaluation type.
 */
std::string GetEvalTypeString(const EvalType eval_type);

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
     * @param enable_et Toggle this flag to enable/disable early termination.
     * @param eval_type The evaluation type for the DPF.
     */
    DpfParameters(const uint32_t n, const uint32_t e,
                  const bool enable_et = true, EvalType eval_type = kOptimizedEvalType);

    /**
     * @brief Get the Input Bitsize object
     * @return uint32_t The size of input in bits.
     */
    uint32_t GetInputBitsize() const {
        return input_bitsize_;
    }

    /**
     * @brief Get the Element Bitsize object
     * @return uint32_t The size of each element in bits.
     */
    uint32_t GetElementBitsize() const {
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
     * @return uint32_t The size of the termination bits.
     */
    uint32_t GetTerminateBitsize() const {
        return terminate_bitsize_;
    }

    /**
     * @brief Get the Full Domain Evaluation type.
     * @return EvalType The evaluation type for full domain evaluation.
     */
    EvalType GetFdeEvalType() const {
        return fde_type_;
    }

    /**
     * @brief Adjust the parameters based on the evaluation type.
     * @param eval_type The evaluation type for the DPF.
     */
    void
    AdjustParameters(EvalType &eval_type);

    /**
     * @brief Set the number of levels to terminate the DPF evaluation.
     */
    void ComputeTerminateLevel();

    /**
     * @brief Set the evaluation type for the DPF.
     * @param eval_type The evaluation type for the DPF.
     */
    void DecideFdeEvalType(EvalType eval_type);

    /**
     * @brief Validate the parameters for the DPF.
     * @return True if the parameters are valid, false otherwise.
     */
    bool ValidateParameters() const;

    /**
     * @brief Reconfigure the parameters for the DPF.
     * @param n The input bitsize.
     * @param e The element bitsize.
     * @param enable_et Toggle this flag to enable/disable early termination.
     * @param eval_type The evaluation type for the DPF.
     */
    void ReconfigureParameters(const uint32_t n, const uint32_t e, const bool enable_et, EvalType eval_type);

    /**
     * @brief Print the details of the DpfParameters.
     */
    void PrintDpfParameters() const;

private:
    uint32_t input_bitsize_;     /**< The size of input in bits. */
    uint32_t element_bitsize_;   /**< The size of each element in bits. */
    bool     enable_et_;         /**< Toggle this flag to enable/disable early termination. */
    uint32_t terminate_bitsize_; /**< The size of the termination bits. */
    EvalType fde_type_;          /**< The evaluation type for full domain evaluation. */
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
