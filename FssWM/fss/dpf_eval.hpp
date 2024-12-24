#ifndef FSS_DPF_EVAL_H_
#define FSS_DPF_EVAL_H_

#include "dpf_key.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

class DpfEvaluator {
public:
    /**
     * @brief Default constructor for DpfEvaluator.
     */
    DpfEvaluator() = delete;

    /**
     * @brief Parameterized constructor for DpfEvaluator.
     * @param params DpfParameters for the DpfKey.
     * @param debug Toggle this flag to enable/disable debugging.
     */
    DpfEvaluator(const DpfParameters &params, const bool debug = false)
        : params_(params), debug_(debug) {
    }

    /**
     * @brief Evaluate the DPF key at the given x value.
     * @param key The DPF key to evaluate.
     * @param x The x value to evaluate the DPF key.
     * @return The evaluated value at x.
     */
    uint32_t EvaluateAt(DpfKey &key, uint32_t x) const;

    /**
     * @brief Evaluate the DPF key at the given x value using the naive approach.
     * @param key The DPF key to evaluate.
     * @param x The x value to evaluate the DPF key.
     * @return The evaluated value at x.
     */
    uint32_t EvaluateAtNaive(DpfKey &key, uint32_t x) const;

    /**
     * @brief Evaluate the DPF key at the given x value using the early termination approach.
     * @param key The DPF key to evaluate.
     * @param x The x value to evaluate the DPF key.
     * @return The evaluated value at x.
     */
    uint32_t EvaluateAtOptimized(DpfKey &key, uint32_t x) const;

private:
    DpfParameters params_; /**< DPF parameters for the DPF key. */
    bool          debug_;  /**< Flag to enable/disable debugging. */

    /**
     * @brief Validate the input values.
     * @param x The x value to evaluate the DPF key.
     * @return True if the input values are valid, false otherwise.
     */
    bool ValidateInput(const uint32_t x) const;

    /**
     * @brief Evaluate the next seed for the DPF key.
     * @param current_level The current level of the DPF key.
     * @param current_seed The current seed of the DPF key.
     * @param current_control_bit The current control bit of the DPF key.
     * @param expanded_seeds The expanded seeds for the DPF key.
     * @param expanded_control_bits The expanded control bits for the DPF key.
     * @param key The DPF key to evaluate.
     */
    void EvaluateNextSeed(
        uint32_t current_level, block &current_seed, bool &current_control_bit,
        std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
        DpfKey &key) const;
};

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm

#endif    // FSS_DPF_EVAL_H_
