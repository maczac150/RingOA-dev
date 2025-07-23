#ifndef FSS_DCF_GEN_H_
#define FSS_DCF_GEN_H_

#include "dcf_key.h"

namespace ringoa {
namespace fss {

namespace prg {

class PseudoRandomGenerator;

}    // namespace prg

namespace dcf {

/**
 * @brief A class to generate a Distributed Point Function (DCF) key.
 */
class DcfKeyGenerator {
public:
    /**
     * @brief Default constructor for DcfKeyGenerator.
     */
    DcfKeyGenerator() = delete;

    /**
     * @brief Parameterized constructor for DcfKeyGenerator.
     * @param params DcfParameters for the DcfKey.
     */
    explicit DcfKeyGenerator(const DcfParameters &params);

    /**
     * @brief Generate a DCF key for the given alpha and beta values.
     * @param alpha The alpha value for the DCF key.
     * @param beta The beta value for the DCF key.
     * @return A pair of DcfKey for the DCF key.
     */
    std::pair<DcfKey, DcfKey> GenerateKeys(uint64_t alpha, uint64_t beta) const;

private:
    DcfParameters               params_; /**< DCF parameters for the DCF key. */
    prg::PseudoRandomGenerator &G_;      /**< PseudoRandomGenerator for the DCF key. */

    /**
     * @brief Validate the input values.
     * @param alpha The alpha value for the DCF key.
     * @param beta The beta value for the DCF key.
     * @return True if the input values are valid, false otherwise.
     */
    bool ValidateInput(const uint64_t alpha, const uint64_t beta) const;
};

}    // namespace dcf
}    // namespace fss
}    // namespace ringoa

#endif    // FSS_DCF_GEN_H_
