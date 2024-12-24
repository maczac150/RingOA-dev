#ifndef FSS_DPF_GEN_H_
#define FSS_DPF_GEN_H_

#include "dpf_key.hpp"

namespace fsswm {
namespace fss {
namespace dpf {

class DpfKeyGenerator {
public:
    /**
     * @brief Default constructor for DpfKeyGenerator.
     */
    DpfKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for DpfKeyGenerator.
     * @param params DpfParameters for the DpfKey.
     * @param debug Toggle this flag to enable/disable debugging.
     */
    DpfKeyGenerator(const DpfParameters &params, const bool debug = false)
        : params_(params), debug_(debug) {
    }

    /**
     * @brief Generate a DPF key for the given alpha and beta values.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @return A pair of DpfKey for the DPF key.
     */
    std::pair<DpfKey, DpfKey> GenerateKeys(uint32_t alpha, uint32_t beta) const;

    /**
     * @brief Generate a DPF key using the naive approach.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @param key_pair The pair of DpfKey for the DPF key.
     */
    void GenerateKeysNaive(uint32_t alpha, uint32_t beta, std::pair<DpfKey, DpfKey> &key_pair) const;

    /**
     * @brief Generate a DPF key using the early termination approach.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @param key_pair The pair of DpfKey for the DPF key.
     */
    void GenerateKeysOptimized(uint32_t alpha, uint32_t beta, std::pair<DpfKey, DpfKey> &key_pair) const;

private:
    DpfParameters params_; /**< DPF parameters for the DPF key. */
    bool          debug_;  /**< Flag to enable/disable debugging. */

    /**
     * @brief Validate the input values.
     * @param alpha The alpha value for the DPF key.
     * @param beta The beta value for the DPF key.
     * @return True if the input values are valid, false otherwise.
     */
    bool ValidateInput(const uint32_t alpha, const uint32_t beta) const;

    /**
     * @brief Generate the next seed for the DPF key.
     * @param current_tree_level The current tree level.
     * @param current_bit The current bit.
     * @param keys The pair of DpfKey for the DPF key.
     * @param current_seeds The current seeds for the DPF key.
     * @param current_control_bits The current control bits for the DPF key.
     */
    void GenerateNextSeed(const uint32_t current_level, const bool current_bit,
                          block &current_seed_0, bool &current_control_bit_0,
                          block &current_seed_1, bool &current_control_bit_1,
                          std::pair<DpfKey, DpfKey> &key_pair) const;
};

}    // namespace dpf
}    // namespace fss
}    // namespace fsswm

#endif    // FSS_DPF_GEN_H_
