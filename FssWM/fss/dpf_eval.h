#ifndef FSS_DPF_EVAL_H_
#define FSS_DPF_EVAL_H_

#include "dpf_key.h"

namespace fsswm {
namespace fss {

namespace prg {

class PseudoRandomGenerator;

}    // namespace prg

namespace dpf {

/**
 * @brief A class to evaluate
 */
class DpfEvaluator {
public:
    /**
     * @brief Default constructor for DpfEvaluator.
     */
    DpfEvaluator() = delete;

    /**
     * @brief Parameterized constructor for DpfEvaluator.
     * @param params DpfParameters for the DpfKey.
     */
    explicit DpfEvaluator(const DpfParameters &params);

    /**
     * @brief Evaluate the DPF key at the given x value.
     * @param key The DPF key to evaluate.
     * @param x The x value to evaluate the DPF key.
     * @return The evaluated value at x.
     */
    uint64_t EvaluateAt(const DpfKey &key, uint64_t x) const;

    /**
     * @brief Evaluate the DPF key for all possible x values.
     * @param keys The DPF keys to evaluate.
     * @param x The x values to evaluate the DPF keys.
     * @param outputs The outputs for the DPF keys.
     */
    void EvaluateAt(const std::vector<DpfKey> &keys, const std::vector<uint64_t> &x, std::vector<uint64_t> &outputs) const;

    /**
     * @brief Evaluate the DPF key for all possible x values.
     * @param key The DPF key to evaluate.
     * @param outputs The outputs for the DPF key.
     */
    void EvaluateFullDomain(const DpfKey &key, std::vector<block> &outputs) const;
    void EvaluateFullDomain(const DpfKey &key, std::vector<uint64_t> &outputs) const;

    block EvaluatePir(const DpfKey &key, std::vector<block> &database) const;

private:
    DpfParameters               params_; /**< DPF parameters for the DPF key. */
    prg::PseudoRandomGenerator &G_;      /**< Pseudo-random generator for the DPF key. */

    /**
     * @brief Validate the input values.
     * @param x The x value to evaluate the DPF key.
     * @return True if the input values are valid, false otherwise.
     */
    bool ValidateInput(const uint64_t x) const;

    /**
     * @brief Evaluate the DPF key at the given x value using the naive approach.
     * @param key The DPF key to evaluate.
     * @param x The x value to evaluate the DPF key.
     * @return The evaluated value at x.
     */
    uint64_t EvaluateAtNaive(const DpfKey &key, uint64_t x) const;

    /**
     * @brief Evaluate the DPF key at the given x value using the optimized approach.
     * @param key The DPF key to evaluate.
     * @param x The x value to evaluate the DPF key.
     * @return The evaluated value at x.
     */
    uint64_t EvaluateAtOptimized(const DpfKey &key, uint64_t x) const;

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
        const uint64_t current_level, const block &current_seed, const bool &current_control_bit,
        std::array<block, 2> &expanded_seeds, std::array<bool, 2> &expanded_control_bits,
        const DpfKey &key) const;

    /**
     * @brief Full domain evaluation of the DPF key using the recursive approach.
     * @param key The DPF key to evaluate.
     * @param outputs The outputs for the DPF key.
     */
    void FullDomainRecursion(const DpfKey &key, std::vector<block> &outputs) const;

    /**
     * @brief Full domain evaluation of the DPF key using the iterative approach with single PRG and parallel evaluation.
     * @param key The DPF key to evaluate.
     * @param outputs The outputs for the DPF key.
     */
    void FullDomainIterativeSingleBatch(const DpfKey &key, std::vector<block> &outputs) const;

    /**
     * @brief Full domain evaluation of the DPF key using the naive approach.
     * @param key The DPF key to evaluate.
     * @param outputs The outputs for the DPF key.
     */
    void FullDomainNaive(const DpfKey &key, std::vector<uint64_t> &outputs) const;

    /**
     * @brief Traverse the DPF key for the given seed and control bit.
     * @param current_seed The current seed for the DPF key.
     * @param current_control_bit The current control bit for the DPF key.
     * @param key The DPF key to evaluate.
     * @param i The current level of the DPF key.
     * @param j The current index of the DPF key.
     * @param outputs The outputs for the DPF key.
     */
    void Traverse(const block &current_seed, const bool current_control_bit, const DpfKey &key, uint64_t i, uint64_t j, std::vector<block> &outputs) const;

    /**
     * @brief Compute the output block for the DPF key.
     * @param seed The seed for the DPF key.
     * @param control_bit The control bit for the DPF key.
     * @param key The DPF key to evaluate.
     * @return block The output block for the DPF key.
     */
    block ComputeOutputBlock(const block &final_seed, bool final_control_bit, const DpfKey &key) const;
};

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm

#endif    // FSS_DPF_EVAL_H_
