#include "binary_2p.h"

#include <cryptoTools/Network/Channel.h>

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace sharing {

BinarySharing2P::BinarySharing2P(const uint64_t bitsize)
    : bitsize_(bitsize), triples_(0), triple_index_(0) {
}

void BinarySharing2P::OfflineSetUp(const uint64_t num_triples, const std::string &file_path) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Offline setup for BinarySharing2P with " + ToString(num_triples) + " triples.");
#endif

    // Generate Beaver triples
    BeaverTriples triples(num_triples);
    GenerateBeaverTriples(num_triples, bitsize_, triples);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generated Beaver triples: " + triples.ToString());
#endif

    // Save Beaver triples
    std::pair<BeaverTriples, BeaverTriples> triples_sh = Share(triples);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Shared Beaver triples");
    Logger::DebugLog(LOC, "Party 0: " + triples_sh.first.ToString());
    Logger::DebugLog(LOC, "Party 1: " + triples_sh.second.ToString());
#endif
    SaveTriplesShareToFile(triples_sh.first, triples_sh.second, file_path);
}

void BinarySharing2P::OnlineSetUp(const uint64_t party_id, const std::string &file_path) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Party " + ToString(party_id) + ": Online setup for BinarySharing2P.");
#endif
    LoadTriplesShareFromFile(party_id, file_path);
}

std::pair<uint64_t, uint64_t> BinarySharing2P::Share(const uint64_t &x) const {
    uint64_t x_0 = GlobalRng::Rand<uint64_t>();
    uint64_t x_1 = x ^ x_0;
    return std::make_pair(x_0, x_1);
}

std::pair<std::array<uint64_t, 2>, std::array<uint64_t, 2>> BinarySharing2P::Share(const std::array<uint64_t, 2> &x) const {
    std::array<uint64_t, 2> x_0, x_1;
    x_0[0] = GlobalRng::Rand<uint64_t>();
    x_0[1] = GlobalRng::Rand<uint64_t>();
    x_1[0] = x[0] ^ x_0[0];
    x_1[1] = x[1] ^ x_0[1];
    return {std::move(x_0), std::move(x_1)};
}

std::pair<std::vector<uint64_t>, std::vector<uint64_t>> BinarySharing2P::Share(const std::vector<uint64_t> &x) const {
    std::vector<uint64_t> x_0(x.size()), x_1(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
        x_0[i] = GlobalRng::Rand<uint64_t>();
        x_1[i] = x[i] ^ x_0[i];
    }
    return {std::move(x_0), std::move(x_1)};
}

std::pair<BeaverTriples, BeaverTriples> BinarySharing2P::Share(const BeaverTriples &triples) const {
    uint64_t      num_triples = triples.num_triples;
    BeaverTriples triples_0(num_triples), triples_1(num_triples);

    for (uint64_t i = 0; i < num_triples; ++i) {
        uint64_t a_0 = GlobalRng::Rand<uint64_t>();
        uint64_t b_0 = GlobalRng::Rand<uint64_t>();
        uint64_t c_0 = GlobalRng::Rand<uint64_t>();

        uint64_t a_1 = triples.triples[i].a ^ a_0;
        uint64_t b_1 = triples.triples[i].b ^ b_0;
        uint64_t c_1 = triples.triples[i].c ^ c_0;

        triples_0.triples[i] = {a_0, b_0, c_0};
        triples_1.triples[i] = {a_1, b_1, c_1};
    }

    return {std::move(triples_0), std::move(triples_1)};
}

void BinarySharing2P::ReconstLocal(const uint64_t &x_0, const uint64_t &x_1, uint64_t &x) const {
    x = x_0 ^ x_1;
}

void BinarySharing2P::ReconstLocal(const std::array<uint64_t, 2> &x_0, const std::array<uint64_t, 2> &x_1, std::array<uint64_t, 2> &x) const {
    x[0] = x_0[0] ^ x_1[0];
    x[1] = x_0[1] ^ x_1[1];
}

void BinarySharing2P::ReconstLocal(const std::vector<uint64_t> &x_0, const std::vector<uint64_t> &x_1, std::vector<uint64_t> &x) const {
    if (x_0.size() != x_1.size()) {
        Logger::ErrorLog(LOC, "Size mismatch between x_0 (" + ToString(x_0.size()) +
                                  ") and x_1 (" + ToString(x_1.size()) + ").");
        return;
    }
    if (x.size() != x_0.size()) {
        x.resize(x_0.size());
    }
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = x_0[i] ^ x_1[i];
    }
}

void BinarySharing2P::ReconstLocal(const BeaverTriples &triples_0, const BeaverTriples &triples_1, BeaverTriples &triples) const {
    if (triples_0.num_triples != triples_1.num_triples) {
        Logger::ErrorLog(LOC, "Size mismatch between triples_0 (" + ToString(triples_0.num_triples) +
                                  ") and triples_1 (" + ToString(triples_1.num_triples) + ").");
        return;
    }
    if (triples.num_triples != triples_0.num_triples) {
        triples.triples.resize(triples_0.num_triples);
    }
    triples.num_triples = triples_0.num_triples;

    for (size_t i = 0; i < triples.num_triples; ++i) {
        triples.triples[i].a = triples_0.triples[i].a ^ triples_1.triples[i].a;
        triples.triples[i].b = triples_0.triples[i].b ^ triples_1.triples[i].b;
        triples.triples[i].c = triples_0.triples[i].c ^ triples_1.triples[i].c;
    }
}

void BinarySharing2P::Reconst(const uint64_t party_id, osuCrypto::Channel &chl, uint64_t &x_0, uint64_t &x_1, uint64_t &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    x = x_0 ^ x_1;
}

void BinarySharing2P::Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::array<uint64_t, 2> &x_0, std::array<uint64_t, 2> &x_1, std::array<uint64_t, 2> &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    x[0] = x_0[0] ^ x_1[0];
    x[1] = x_0[1] ^ x_1[1];
}

void BinarySharing2P::Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::array<uint64_t, 4> &x_0, std::array<uint64_t, 4> &x_1, std::array<uint64_t, 4> &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    x[0] = x_0[0] ^ x_1[0];
    x[1] = x_0[1] ^ x_1[1];
    x[2] = x_0[2] ^ x_1[2];
    x[3] = x_0[3] ^ x_1[3];
}

void BinarySharing2P::Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::vector<uint64_t> &x_0, std::vector<uint64_t> &x_1, std::vector<uint64_t> &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    if (x.size() != x_0.size()) {
        x.resize(x_0.size());
    }
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = x_0[i] ^ x_1[i];
    }
}

void BinarySharing2P::Reconst(const uint64_t party_id, osuCrypto::Channel &chl, std::array<std::vector<uint64_t>, 2> &x_0, std::array<std::vector<uint64_t>, 2> &x_1, std::array<std::vector<uint64_t>, 2> &x) const {
    if (party_id == 0) {
        chl.send(x_0[0]);
        chl.send(x_0[1]);
        chl.recv(x_1[0]);
        chl.recv(x_1[1]);
    } else {
        chl.recv(x_0[0]);
        chl.recv(x_0[1]);
        chl.send(x_1[0]);
        chl.send(x_1[1]);
    }
    if (x_0[0].size() != x_1[0].size() || x_0[1].size() != x_1[1].size()) {
        Logger::ErrorLog(LOC, "Size mismatch between x_0 and x_1.");
        return;
    }
    // Resize x if necessary
    if (x[0].size() != x_0[0].size()) {
        x[0].resize(x_0[0].size());
        x[1].resize(x_0[1].size());
    }
    for (size_t i = 0; i < x[0].size(); ++i) {
        x[0][i] = x_0[0][i] ^ x_1[0][i];
        x[1][i] = x_0[1][i] ^ x_1[1][i];
    }
}

void BinarySharing2P::EvaluateXor(const uint64_t &x, const uint64_t &y, uint64_t &z) const {
    z = x ^ y;
}

void BinarySharing2P::EvaluateXor(const std::array<uint64_t, 2> &x, const std::array<uint64_t, 2> &y, std::array<uint64_t, 2> &z) const {
    z[0] = x[0] ^ y[0];
    z[1] = x[1] ^ y[1];
}

void BinarySharing2P::EvaluateXor(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, std::vector<uint64_t> &z) const {
    if (x.size() != y.size()) {
        Logger::ErrorLog(LOC, "Size mismatch: x.size() != y.size() in EvaluateXor.");
        return;
    }
    if (z.size() != x.size()) {
        z.resize(x.size());
    }
    for (size_t i = 0; i < x.size(); ++i) {
        z[i] = x[i] ^ y[i];
    }
}

void BinarySharing2P::EvaluateAnd(const uint64_t party_id, osuCrypto::Channel &chl, const uint64_t &x, const uint64_t &y, uint64_t &z) {
    if (triple_index_ >= triples_.num_triples) {
        Logger::ErrorLog(LOC, "No more Beaver triples available.");
        return;
    }

    // Get the current Beaver triple
    const auto &triple = triples_.triples[triple_index_];
    ++triple_index_;

    // -------------------------------------------------------
    // 1) Prepare local differences: d = (x ^ a), e = (y ^ b)
    //    Then we reconstruct d, e across both parties.
    // -------------------------------------------------------

    // For convenience, define local arrays for d, e
    // We'll store them as:   de_0 = { d, e } on Party 0
    //                        de_1 = { d, e } on Party 1
    // Then we 'Reconst' them into a single array 'de' = { d_reconst, e_reconst }

    // We'll define them as std::array<T, 2> so that each of d, e is of type T.
    std::array<uint64_t, 2> de_0, de_1;

    if (party_id == 0) {
        de_0[0] = x ^ triple.a;    // d
        de_0[1] = y ^ triple.b;    // e
    } else {
        de_1[0] = x ^ triple.a;    // d
        de_1[1] = y ^ triple.b;    // e
    }

    // -------------------------------------------------------
    // 2) Reconstruct d, e across both parties
    //    We'll store it in 'de': { d_reconst, e_reconst }
    // -------------------------------------------------------
    std::array<uint64_t, 2> de;    // d_reconst = de[0], e_reconst = de[1]

    Reconst(party_id, chl, de_0, de_1, de);

    // -------------------------------------------------------
    // 3) Compute final product share z
    //    Beaver formula: z = a&e ^ b&d ^ c (^ d&e for party 0)
    // -------------------------------------------------------
    if (party_id == 0) {
        z = (de[1] & triple.a) ^    // a & e
            (de[0] & triple.b) ^    // b & d
            triple.c ^              // c
            (de[0] & de[1]);        // d & e
    } else {
        z = (de[1] & triple.a) ^
            (de[0] & triple.b) ^
            triple.c;
    }
}

void BinarySharing2P::EvaluateAnd(const uint64_t party_id, osuCrypto::Channel &chl, const std::array<uint64_t, 2> &x, const std::array<uint64_t, 2> &y, std::array<uint64_t, 2> &z) {
    // Use two different Beaver triples for each element of array x, y
    if (triple_index_ + 1 >= triples_.num_triples) {
        Logger::ErrorLog(LOC, "No more Beaver triples available.");
        return;
    }

    // Get the current Beaver triples
    const auto &triple_0 = triples_.triples[triple_index_];
    const auto &triple_1 = triples_.triples[triple_index_ + 1];
    triple_index_ += 2;

    // -------------------------------------------------------
    // 1) Prepare local differences: d = (x ^ a), e = (y ^ b)
    //    Then we reconstruct d, e across both parties.
    // -------------------------------------------------------

    // For convenience, define local arrays for d, e
    // We'll store them as:   de_0 = { d0_0, e0_0, d1_0, e1_0 } on Party 0
    //                        de_1 = { d0_1, e0_1, d1_1, e1_1 } on Party 1
    // Then we 'Reconst' them into a single array 'de' = { d0_reconst, e0_reconst, d1_reconst, e1_reconst }

    // We'll define them as std::array<T, 4> so that each of d, e is of type T.
    std::array<uint64_t, 4> de_0, de_1;

    if (party_id == 0) {
        de_0[0] = x[0] ^ triple_0.a;    // d0
        de_0[1] = y[0] ^ triple_0.b;    // e0
        de_0[2] = x[1] ^ triple_1.a;    // d1
        de_0[3] = y[1] ^ triple_1.b;    // e1
    } else {
        de_1[0] = x[0] ^ triple_0.a;    // d0
        de_1[1] = y[0] ^ triple_0.b;    // e0
        de_1[2] = x[1] ^ triple_1.a;    // d1
        de_1[3] = y[1] ^ triple_1.b;    // e1
    }

    // -------------------------------------------------------
    // 2) Reconstruct d, e across both parties
    //    We'll store it in 'de': { d0_reconst, e0_reconst, d1_reconst, e1_reconst }
    // -------------------------------------------------------
    std::array<uint64_t, 4> de;    // d0_reconst = de[0], e0_reconst = de[1], d1_reconst = de[2], e1_reconst = de[3]
    Reconst(party_id, chl, de_0, de_1, de);

    // -------------------------------------------------------
    // 3) Compute final product share z
    //    Beaver formula: z = a&e ^ b&d ^ c (^ d&e for party 0)
    // -------------------------------------------------------
    if (party_id == 0) {
        z[0] = (de[1] & triple_0.a) ^    // a & e0
               (de[0] & triple_0.b) ^    // b & d0
               triple_0.c ^              // c0
               (de[0] & de[1]);          // d0 & e0
        z[1] = (de[3] & triple_1.a) ^    // a & e1
               (de[2] & triple_1.b) ^    // b & d1
               triple_1.c ^              // c1
               (de[2] & de[3]);          // d1 & e1
    } else {
        z[0] = (de[1] & triple_0.a) ^
               (de[0] & triple_0.b) ^
               triple_0.c;
        z[1] = (de[3] & triple_1.a) ^
               (de[2] & triple_1.b) ^
               triple_1.c;
    }
}

uint64_t BinarySharing2P::GenerateRandomValue() const {
    return Mod(GlobalRng::Rand<uint64_t>(), bitsize_);
}

void BinarySharing2P::PrintTriples(const size_t limit) const {
    Logger::DebugLog(LOC, "Beaver triples:" + triples_.ToString(limit));
}

uint64_t BinarySharing2P::GetBitSize() const {
    return bitsize_;
}

uint64_t BinarySharing2P::GetCurrentTripleIndex() const {
    return triple_index_;
}

uint64_t BinarySharing2P::GetNumTriples() const {
    return triples_.num_triples;
}

uint64_t BinarySharing2P::GetRemainingTripleCount() const {
    return triples_.num_triples - triple_index_;
}

// ----------------------------------------------------
// Internal functions
// ----------------------------------------------------

void BinarySharing2P::GenerateBeaverTriples(const uint64_t num_triples, const uint64_t bitsize, BeaverTriples &triples) const {
    if (num_triples == 0) {
        Logger::ErrorLog(LOC, "Number of triples must be greater than 0.");
        return;
    }
    if (triples.num_triples != num_triples) {
        triples.triples.resize(num_triples);
    }
    for (uint64_t i = 0; i < num_triples; ++i) {
        uint64_t a         = GlobalRng::Rand<uint64_t>();
        uint64_t b         = GlobalRng::Rand<uint64_t>() % bitsize;
        uint64_t c         = (a & b) % bitsize;
        triples.triples[i] = {a, b, c};
    }
}

void BinarySharing2P::SaveTriplesShareToFile(const BeaverTriples &triples_0, const BeaverTriples &triples_1, const std::string &file_path) const {
    std::vector<uint8_t> buffer_0, buffer_1;
    triples_0.Serialize(buffer_0);
    triples_1.Serialize(buffer_1);

    FileIo io(".bt.bin");
    io.WriteBinary(file_path + "_0", buffer_0);
    io.WriteBinary(file_path + "_1", buffer_1);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Beaver triples saved successfully to " + file_path + io.GetExtension());
#endif
}

void BinarySharing2P::LoadTriplesShareFromFile(const uint64_t party_id, const std::string &file_path) {
    std::vector<uint8_t> buffer;
    FileIo               io(".bt.bin");
    io.ReadBinary(file_path + "_" + ToString(party_id), buffer);

    BeaverTriples triples;
    triples.Deserialize(buffer);
    triples_ = std::move(triples);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Beaver triples loaded successfully from " + file_path + io.GetExtension());
#endif
}

}    // namespace sharing
}    // namespace fsswm
