#ifndef SHARING_ADDITIVE_2P_H_
#define SHARING_ADDITIVE_2P_H_

#include <array>
#include <string>

#include "beaver_triples.h"

// Forward declaration
namespace osuCrypto {

class Channel;

}    // namespace osuCrypto

namespace ringoa {
namespace sharing {

class AdditiveSharing2P {
public:
    AdditiveSharing2P() = delete;
    explicit AdditiveSharing2P(const uint64_t bitsize);

    // SetUp functions (You need to call these functions before using secure multiplication)
    void OfflineSetUp(const uint64_t num_triples, const std::string &file_path) const;
    void OnlineSetUp(const uint64_t party_id, const std::string &file_path);

    // Share data (single value, std::array, std::vector, BeaverTriples)
    std::pair<uint64_t, uint64_t>                               Share(const uint64_t &x) const;
    std::pair<std::array<uint64_t, 2>, std::array<uint64_t, 2>> Share(const std::array<uint64_t, 2> &x) const;
    std::pair<std::vector<uint64_t>, std::vector<uint64_t>>     Share(const std::vector<uint64_t> &x) const;
    std::pair<BeaverTriples, BeaverTriples>                     Share(const BeaverTriples &triples) const;

    // Reconstruct data without interaction (single value, std::array, std::vector, BeaverTriples)
    void ReconstLocal(const uint64_t &x_0, const uint64_t &x_1, uint64_t &x) const;
    void ReconstLocal(const std::array<uint64_t, 2> &x_0, const std::array<uint64_t, 2> &x_1, std::array<uint64_t, 2> &x) const;
    void ReconstLocal(const std::vector<uint64_t> &x_0, const std::vector<uint64_t> &x_1, std::vector<uint64_t> &x) const;
    void ReconstLocal(const BeaverTriples &triples_0, const BeaverTriples &triples_1, BeaverTriples &triples) const;

    // Reconstruct data with interaction (single value, std::array, std::vector, std::array<std::vector, 2>)
    void Reconst(const uint64_t party_id, osuCrypto::Channel &chl, uint64_t &x_0, uint64_t &x_1, uint64_t &x) const;
    void Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::array<uint64_t, 2> &x_0, std::array<uint64_t, 2> &x_1, std::array<uint64_t, 2> &x) const;
    void Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::array<uint64_t, 4> &x_0, std::array<uint64_t, 4> &x_1, std::array<uint64_t, 4> &x) const;
    void Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::vector<uint64_t> &x_0, std::vector<uint64_t> &x_1, std::vector<uint64_t> &x) const;
    void Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::array<std::vector<uint64_t>, 2> &x_0, std::array<std::vector<uint64_t>, 2> &x_1, std::array<std::vector<uint64_t>, 2> &x) const;

    // Evaluate operations (addition, subtraction, multiplication, selection)
    void EvaluateAdd(const uint64_t &x, const uint64_t &y, uint64_t &z) const;
    void EvaluateAdd(const std::array<uint64_t, 2> &x, const std::array<uint64_t, 2> &y, std::array<uint64_t, 2> &z) const;
    void EvaluateAdd(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, std::vector<uint64_t> &z) const;

    void EvaluateSub(const uint64_t &x, const uint64_t &y, uint64_t &z) const;
    void EvaluateSub(const std::array<uint64_t, 2> &x, const std::array<uint64_t, 2> &y, std::array<uint64_t, 2> &z) const;
    void EvaluateSub(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, std::vector<uint64_t> &z) const;

    void EvaluateMult(const uint64_t party_id, osuCrypto::Channel &chl, const uint64_t &x, const uint64_t &y, uint64_t &z);
    void EvaluateMult(const uint64_t party_id, osuCrypto::Channel &chl, const std::array<uint64_t, 2> &x, const std::array<uint64_t, 2> &y, std::array<uint64_t, 2> &z);
    // void EvaluateMult(const uint64_t party_id, osuCrypto::Channel &chl, const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, std::vector<uint64_t> &z);

    void EvaluateSelect(const uint64_t party_id, osuCrypto::Channel &chl, const uint64_t &x, const uint64_t &y, const uint64_t &c, uint64_t &z);
    void EvaluateSelect(const uint64_t party_id, osuCrypto::Channel &chl, const std::array<uint64_t, 2> &x, const std::array<uint64_t, 2> &y, const std::array<uint64_t, 2> &c, std::array<uint64_t, 2> &z);
    // void EvaluateSelect(const uint64_t party_id, osuCrypto::Channel &chl, const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, const std::vector<uint64_t> &c, std::vector<uint64_t> &z);

    uint64_t GenerateRandomValue() const;
    void     PrintTriples(const size_t limit = 0) const;
    uint64_t GetBitSize() const;
    uint64_t GetCurrentTripleIndex() const;
    uint64_t GetNumTriples() const;
    uint64_t GetRemainingTripleCount() const;
    void     ResetTripleIndex();

private:
    const uint64_t bitsize_;      /**< The size of the bits used for secret sharing operations. */
    BeaverTriples  triples_;      /**< The Beaver triples used for secure multiplication. */
    uint64_t       triple_index_; /**< The index of the current Beaver triple. */

    // Internal functions
    void GenerateBeaverTriples(const uint64_t num_triples, const uint64_t bitsize, BeaverTriples &triples) const;
    void SaveTriplesShareToFile(const BeaverTriples &triples_0, const BeaverTriples &triples_1, const std::string &file_path) const;
    void LoadTriplesShareFromFile(const uint64_t party_id, const std::string &file_path);
};

}    // namespace sharing
}    // namespace ringoa

#endif
