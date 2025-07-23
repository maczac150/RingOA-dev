#ifndef FSS_DCF_EVAL_H_
#define FSS_DCF_EVAL_H_

#include "dcf_key.h"

namespace ringoa {
namespace fss {

namespace prg {

class PseudoRandomGenerator;

}    // namespace prg

namespace dcf {

/**
 * @brief A class to evaluate Distributed Comparison Function (DCF) keys.
 */
class DcfEvaluator {
public:
    /**
     * @brief Default constructor for DcfEvaluator.
     */
    DcfEvaluator() = delete;

    /**
     * @brief Parameterized constructor for DcfEvaluator.
     * @param params DcfParameters for the DcfKey.
     */
    explicit DcfEvaluator(const DcfParameters &params);

    /**
     * @brief Evaluate the DCF key at the given x value.
     * @param key The DCF key to evaluate.
     * @param x The x value to evaluate the DCF key.
     * @return The evaluated value at x.
     */
    uint64_t EvaluateAt(const DcfKey &key, uint64_t x) const;

private:
    DcfParameters               params_; /**< DCF parameters for the DCF key. */
    prg::PseudoRandomGenerator &G_;      /**< Pseudo-random generator for the DCF key. */

    /**
     * @brief Validate the input values.
     * @param x The x value to evaluate the DCF key.
     * @return True if the input values are valid, false otherwise.
     */
    bool ValidateInput(const uint64_t x) const;

    /**
     * @brief Evaluate the next seed for the DCF key.
     * @param current_level The current level of the DCF key.
     * @param current_seed The current seed of the DCF key.
     * @param current_control_bit The current control bit of the DCF key.
     * @param expanded_seeds The expanded seeds for the DCF key.
     * @param expanded_values The expanded values for the DCF key.
     * @param expanded_control_bits The expanded control bits for the DCF key.
     * @param key The DCF key to evaluate.
     */
    void EvaluateNextSeed(
        const uint64_t current_level, const block &current_seed, const bool &current_control_bit,
        std::array<block, 2> &expanded_seeds, std::array<block, 2> &expanded_values, std::array<bool, 2> &expanded_control_bits,
        const DcfKey &key) const;
};

}    // namespace dcf
}    // namespace fss
}    // namespace ringoa

#endif    // FSS_DCF_EVAL_H_
