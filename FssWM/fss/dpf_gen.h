#ifndef FSS_DPF_GEN_H_
#define FSS_DPF_GEN_H_

#include "dpf_key.h"

namespace fsswm {
namespace fss {

namespace prg {

class PseudoRandomGenerator;

}    // namespace prg

namespace dpf {

/**
 * @brief A class to generate a Distributed Point Function (DPF) key.
 */
class DpfKeyGenerator {
public:
    /**
     * @brief Default constructor for DpfKeyGenerator.
     */
    DpfKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for DpfKeyGenerator.
     * @param params DpfParameters for the DpfKey.
     */
    explicit DpfKeyGenerator(const DpfParameters &params);

    /**
     * @brief Generate a DPF key for the given alpha and beta values.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @return A pair of DpfKey for the DPF key.
     */
    std::pair<DpfKey, DpfKey> GenerateKeys(uint64_t alpha, uint64_t beta) const;

    /**
     * @brief Generate a DPF key using the naive approach.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @param key_pair The pair of DpfKey for the DPF key.
     */
    void GenerateKeysNaive(uint64_t alpha, uint64_t beta, std::pair<DpfKey, DpfKey> &key_pair) const;

    /**
     * @brief Generate a DPF key using the early termination approach.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @param key_pair The pair of DpfKey for the DPF key.
     */
    void GenerateKeysOptimized(uint64_t alpha, uint64_t beta, std::pair<DpfKey, DpfKey> &key_pair) const;

private:
    DpfParameters               params_; /**< DPF parameters for the DPF key. */
    prg::PseudoRandomGenerator &G_;      /**< PseudoRandomGenerator for the DPF key. */

    /**
     * @brief Validate the input values.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @return True if the input values are valid, false otherwise.
     */
    bool ValidateInput(const uint64_t alpha, const uint64_t beta) const;

    /**
     * @brief Generate the next seed for the DPF key.
     * @param current_tree_level The current tree level.
     * @param current_bit The current bit.
     * @param keys The pair of DpfKey for the DPF key.
     * @param current_seeds The current seeds for the DPF key.
     * @param current_control_bits The current control bits for the DPF key.
     */
    void GenerateNextSeed(const uint64_t current_level, const bool current_bit,
                          block &current_seed_0, bool &current_control_bit_0,
                          block &current_seed_1, bool &current_control_bit_1,
                          std::pair<DpfKey, DpfKey> &key_pair) const;

    /**
     * @brief Set the output for the DPF key.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @param final_seed_0 The seed for the DPF key for party 0.
     * @param final_seed_1 The seed for the DPF key for party 1.
     * @param final_control_bit_1 The control bit for party 1.
     * @param key_pair The pair of DpfKey for the DPF key.
     */
    void ComputeAdditiveShiftedOutput(uint64_t alpha, uint64_t beta,
                           block &final_seed_0, block &final_seed_1, bool final_control_bit_1,
                           std::pair<DpfKey, DpfKey> &key_pair) const;

    void ComputeSingleBitMaskOutput(uint64_t alpha, block &final_seed_0, block &final_seed_1,
                              std::pair<DpfKey, DpfKey> &key_pair) const;
};

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm

#endif    // FSS_DPF_GEN_H_
