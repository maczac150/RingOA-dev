#ifndef SHARING_ADDITIVE_2P_H_
#define SHARING_ADDITIVE_2P_H_

#include <array>
#include <string>

#include "cryptoTools/Crypto/PRNG.h"

#include "beaver_triples.h"

// Forward declaration
namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace fsswm {
namespace sharing {

class AdditiveSharing2P {
public:
    AdditiveSharing2P() = delete;
    explicit AdditiveSharing2P(const uint32_t bitsize);

    // SetUp functions (You need to call these functions before using secure multiplication)
    void OfflineSetUp(const uint32_t num_triples, const std::string &file_path) const;
    void OnlineSetUp(const uint32_t party_id, const std::string &file_path);

    // Share data (single value, std::array, std::vector, BeaverTriples)
    std::pair<uint32_t, uint32_t>                               Share(const uint32_t &x) const;
    std::pair<std::array<uint32_t, 2>, std::array<uint32_t, 2>> Share(const std::array<uint32_t, 2> &x) const;
    std::pair<std::vector<uint32_t>, std::vector<uint32_t>>     Share(const std::vector<uint32_t> &x) const;
    std::pair<BeaverTriples, BeaverTriples>                     Share(const BeaverTriples &triples) const;

    // Reconstruct data without interaction (single value, std::array, std::vector, BeaverTriples)
    void ReconstLocal(const uint32_t &x_0, const uint32_t &x_1, uint32_t &x) const;
    void ReconstLocal(const std::array<uint32_t, 2> &x_0, const std::array<uint32_t, 2> &x_1, std::array<uint32_t, 2> &x) const;
    void ReconstLocal(const std::vector<uint32_t> &x_0, const std::vector<uint32_t> &x_1, std::vector<uint32_t> &x) const;
    void ReconstLocal(const BeaverTriples &triples_0, const BeaverTriples &triples_1, BeaverTriples &triples) const;

    // Reconstruct data with interaction (single value, std::array, std::vector, std::array<std::vector, 2>)
    void Reconst(const uint32_t party_id, osuCrypto::Channel &chl, uint32_t &x_0, uint32_t &x_1, uint32_t &x) const;
    void Reconst(const uint32_t party_id, osuCrypto::Channel &chl, std::array<uint32_t, 2> &x_0, std::array<uint32_t, 2> &x_1, std::array<uint32_t, 2> &x) const;
    void Reconst(const uint32_t party_id, osuCrypto::Channel &chl, std::array<uint32_t, 4> &x_0, std::array<uint32_t, 4> &x_1, std::array<uint32_t, 4> &x) const;
    void Reconst(const uint32_t party_id, osuCrypto::Channel &chl, std::vector<uint32_t> &x_0, std::vector<uint32_t> &x_1, std::vector<uint32_t> &x) const;
    void Reconst(const uint32_t party_id, osuCrypto::Channel &chl, std::array<std::vector<uint32_t>, 2> &x_0, std::array<std::vector<uint32_t>, 2> &x_1, std::array<std::vector<uint32_t>, 2> &x) const;

    // Evaluate operations (addition, subtraction, multiplication, selection)
    void EvaluateAdd(const uint32_t &x, const uint32_t &y, uint32_t &z) const;
    void EvaluateAdd(const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, std::array<uint32_t, 2> &z) const;
    void EvaluateAdd(const std::vector<uint32_t> &x, const std::vector<uint32_t> &y, std::vector<uint32_t> &z) const;

    void EvaluateSub(const uint32_t &x, const uint32_t &y, uint32_t &z) const;
    void EvaluateSub(const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, std::array<uint32_t, 2> &z) const;
    void EvaluateSub(const std::vector<uint32_t> &x, const std::vector<uint32_t> &y, std::vector<uint32_t> &z) const;

    void EvaluateMult(const uint32_t party_id, osuCrypto::Channel &chl, const uint32_t &x, const uint32_t &y, uint32_t &z);
    void EvaluateMult(const uint32_t party_id, osuCrypto::Channel &chl, const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, std::array<uint32_t, 2> &z);
    // void EvaluateMult(const uint32_t party_id, osuCrypto::Channel &chl, const std::vector<uint32_t> &x, const std::vector<uint32_t> &y, std::vector<uint32_t> &z);

    void EvaluateSelect(const uint32_t party_id, osuCrypto::Channel &chl, const uint32_t &x, const uint32_t &y, const uint32_t &c, uint32_t &z);
    void EvaluateSelect(const uint32_t party_id, osuCrypto::Channel &chl, const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, const std::array<uint32_t, 2> &c, std::array<uint32_t, 2> &z);
    // void EvaluateSelect(const uint32_t party_id, osuCrypto::Channel &chl, const std::vector<uint32_t> &x, const std::vector<uint32_t> &y, const std::vector<uint32_t> &c, std::vector<uint32_t> &z);

    uint32_t GenerateRandomValue() const;
    void     PrintTriples() const;
    uint32_t GetBitSize() const;
    uint32_t GetCurrentTripleIndex() const;
    uint32_t GetNumTriples() const;
    uint32_t GetRemainingTripleCount() const;

private:
    const uint32_t   bitsize_;      /**< The size of the bits used for secret sharing operations. */
    BeaverTriples    triples_;      /**< The Beaver triples used for secure multiplication. */
    uint32_t         triple_index_; /**< The index of the current Beaver triple. */
    mutable oc::PRNG prng_;         /**< The PRNG used for generating random values. */

    // Internal functions
    void GenerateBeaverTriples(const uint32_t num_triples, const uint32_t bitsize, BeaverTriples &triples) const;
    void SaveTriplesShareToFile(const BeaverTriples &triples_0, const BeaverTriples &triples_1, const std::string &file_path) const;
    void LoadTriplesShareFromFile(const uint32_t party_id, const std::string &file_path);
};

}    // namespace sharing
}    // namespace fsswm

#endif
