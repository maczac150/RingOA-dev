#include "additive_2p.h"

#include "cryptoTools/Network/Channel.h"

#include "FssWM/utils/file_io.h"
#include "FssWM/utils/logger.h"
#include "FssWM/utils/rng.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace sharing {

AdditiveSharing2P::AdditiveSharing2P(const uint32_t bitsize)
    : bitsize_(bitsize), triples_(0), triple_index_(0) {
    prng_.SetSeed(oc::toBlock(SecureRng::Rand64(), SecureRng::Rand64()));
}

void AdditiveSharing2P::OfflineSetUp(const uint32_t num_triples, const std::string &file_path) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Offline setup for AdditiveSharing2P with " + std::to_string(num_triples) + " triples.");
#endif

    // Generate Beaver triples
    BeaverTriples triples(num_triples);
    GenerateBeaverTriples(num_triples, bitsize_, triples);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Generated Beaver triples");
    triples.DebugLog();
#endif

    // Save Beaver triples
    std::pair<BeaverTriples, BeaverTriples> triples_sh = Share(triples);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Shared Beaver triples");
    Logger::DebugLog(LOC, "Party 0:");
    triples_sh.first.DebugLog();
    Logger::DebugLog(LOC, "Party 1:");
    triples_sh.second.DebugLog();
#endif
    SaveTriplesShareToFile(triples_sh.first, triples_sh.second, file_path);
}

void AdditiveSharing2P::OnlineSetUp(const uint32_t party_id, const std::string &file_path) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Party " + std::to_string(party_id) + ": Online setup for AdditiveSharing2P.");
#endif
    LoadTriplesShareFromFile(party_id, file_path);
}

std::pair<uint32_t, uint32_t> AdditiveSharing2P::Share(const uint32_t &x) const {
    uint32_t x_0 = Mod(prng_.get<uint32_t>(), bitsize_);
    uint32_t x_1 = Mod(x - x_0, bitsize_);
    return std::make_pair(x_0, x_1);
}

std::pair<std::array<uint32_t, 2>, std::array<uint32_t, 2>> AdditiveSharing2P::Share(const std::array<uint32_t, 2> &x) const {
    std::array<uint32_t, 2> x_0, x_1;
    x_0[0] = Mod(prng_.get<uint32_t>(), bitsize_);
    x_0[1] = Mod(prng_.get<uint32_t>(), bitsize_);
    x_1[0] = Mod(x[0] - x_0[0], bitsize_);
    x_1[1] = Mod(x[1] - x_0[1], bitsize_);
    return {std::move(x_0), std::move(x_1)};
}

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> AdditiveSharing2P::Share(const std::vector<uint32_t> &x) const {
    std::vector<uint32_t> x_0(x.size()), x_1(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
        x_0[i] = Mod(prng_.get<uint32_t>(), bitsize_);
        x_1[i] = Mod(x[i] - x_0[i], bitsize_);
    }
    return {std::move(x_0), std::move(x_1)};
}

std::pair<BeaverTriples, BeaverTriples> AdditiveSharing2P::Share(const BeaverTriples &triples) const {
    uint32_t      num_triples = triples.num_triples;
    BeaverTriples triples_0(num_triples), triples_1(num_triples);

    for (uint32_t i = 0; i < num_triples; ++i) {
        uint32_t a_0 = Mod(prng_.get<uint32_t>(), bitsize_);
        uint32_t b_0 = Mod(prng_.get<uint32_t>(), bitsize_);
        uint32_t c_0 = Mod(prng_.get<uint32_t>(), bitsize_);

        uint32_t a_1 = Mod(triples.triples[i].a - a_0, bitsize_);
        uint32_t b_1 = Mod(triples.triples[i].b - b_0, bitsize_);
        uint32_t c_1 = Mod(triples.triples[i].c - c_0, bitsize_);

        triples_0.triples[i] = {a_0, b_0, c_0};
        triples_1.triples[i] = {a_1, b_1, c_1};
    }

    return {std::move(triples_0), std::move(triples_1)};
}

void AdditiveSharing2P::ReconstLocal(const uint32_t &x_0, const uint32_t &x_1, uint32_t &x) const {
    x = Mod(x_0 + x_1, bitsize_);
}

void AdditiveSharing2P::ReconstLocal(const std::array<uint32_t, 2> &x_0, const std::array<uint32_t, 2> &x_1, std::array<uint32_t, 2> &x) const {
    x[0] = Mod(x_0[0] + x_1[0], bitsize_);
    x[1] = Mod(x_0[1] + x_1[1], bitsize_);
}

void AdditiveSharing2P::ReconstLocal(const std::vector<uint32_t> &x_0, const std::vector<uint32_t> &x_1, std::vector<uint32_t> &x) const {
    if (x_0.size() != x_1.size()) {
        Logger::ErrorLog(LOC, "Size mismatch between x_0 (" + std::to_string(x_0.size()) +
                                  ") and x_1 (" + std::to_string(x_1.size()) + ").");
        return;
    }
    if (x.size() != x_0.size()) {
        x.resize(x_0.size());
    }
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = Mod(x_0[i] + x_1[i], bitsize_);
    }
}

void AdditiveSharing2P::ReconstLocal(const BeaverTriples &triples_0, const BeaverTriples &triples_1, BeaverTriples &triples) const {
    if (triples_0.num_triples != triples_1.num_triples) {
        Logger::ErrorLog(LOC, "Size mismatch between triples_0 (" + std::to_string(triples_0.num_triples) +
                                  ") and triples_1 (" + std::to_string(triples_1.num_triples) + ").");
        return;
    }
    if (triples.num_triples != triples_0.num_triples) {
        triples.triples.resize(triples_0.num_triples);
    }
    triples.num_triples = triples_0.num_triples;

    auto add_mod = [this](uint32_t a, uint32_t b) { return Mod(a + b, bitsize_); };

    for (size_t i = 0; i < triples.num_triples; ++i) {
        triples.triples[i].a = add_mod(triples_0.triples[i].a, triples_1.triples[i].a);
        triples.triples[i].b = add_mod(triples_0.triples[i].b, triples_1.triples[i].b);
        triples.triples[i].c = add_mod(triples_0.triples[i].c, triples_1.triples[i].c);
    }
}

void AdditiveSharing2P::Reconst(const uint32_t party_id, oc::Channel &chl, uint32_t &x_0, uint32_t &x_1, uint32_t &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    x = Mod(x_0 + x_1, bitsize_);
}

void AdditiveSharing2P::Reconst(const uint32_t party_id, oc::Channel &chl, std::array<uint32_t, 2> &x_0, std::array<uint32_t, 2> &x_1, std::array<uint32_t, 2> &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    x[0] = Mod(x_0[0] + x_1[0], bitsize_);
    x[1] = Mod(x_0[1] + x_1[1], bitsize_);
}

void AdditiveSharing2P::Reconst(const uint32_t party_id, oc::Channel &chl, std::array<uint32_t, 4> &x_0, std::array<uint32_t, 4> &x_1, std::array<uint32_t, 4> &x) const {
    if (party_id == 0) {
        chl.send(x_0);
        chl.recv(x_1);
    } else {
        chl.recv(x_0);
        chl.send(x_1);
    }
    x[0] = Mod(x_0[0] + x_1[0], bitsize_);
    x[1] = Mod(x_0[1] + x_1[1], bitsize_);
    x[2] = Mod(x_0[2] + x_1[2], bitsize_);
    x[3] = Mod(x_0[3] + x_1[3], bitsize_);
}

void AdditiveSharing2P::Reconst(const uint32_t party_id, oc::Channel &chl, std::vector<uint32_t> &x_0, std::vector<uint32_t> &x_1, std::vector<uint32_t> &x) const {
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
        x[i] = Mod(x_0[i] + x_1[i], bitsize_);
    }
}

void AdditiveSharing2P::Reconst(const uint32_t party_id, oc::Channel &chl, std::array<std::vector<uint32_t>, 2> &x_0, std::array<std::vector<uint32_t>, 2> &x_1, std::array<std::vector<uint32_t>, 2> &x) const {
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
        x[0][i] = Mod(x_0[0][i] + x_1[0][i], bitsize_);
        x[1][i] = Mod(x_0[1][i] + x_1[1][i], bitsize_);
    }
}

void AdditiveSharing2P::EvaluateAdd(const uint32_t &x, const uint32_t &y, uint32_t &z) const {
    z = Mod(x + y, bitsize_);
}

void AdditiveSharing2P::EvaluateAdd(const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, std::array<uint32_t, 2> &z) const {
    z[0] = Mod(x[0] + y[0], bitsize_);
    z[1] = Mod(x[1] + y[1], bitsize_);
}

void AdditiveSharing2P::EvaluateAdd(const std::vector<uint32_t> &x, const std::vector<uint32_t> &y, std::vector<uint32_t> &z) const {
    if (x.size() != y.size()) {
        Logger::ErrorLog(LOC, "Size mismatch: x.size() != y.size() in EvaluateAdd.");
        return;
    }
    if (z.size() != x.size()) {
        z.resize(x.size());
    }
    for (size_t i = 0; i < x.size(); ++i) {
        z[i] = Mod(x[i] + y[i], bitsize_);
    }
}

void AdditiveSharing2P::EvaluateSub(const uint32_t &x, const uint32_t &y, uint32_t &z) const {
    z = Mod(x - y, bitsize_);
}

void AdditiveSharing2P::EvaluateSub(const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, std::array<uint32_t, 2> &z) const {
    z[0] = Mod(x[0] - y[0], bitsize_);
    z[1] = Mod(x[1] - y[1], bitsize_);
}

void AdditiveSharing2P::EvaluateSub(const std::vector<uint32_t> &x, const std::vector<uint32_t> &y, std::vector<uint32_t> &z) const {
    if (x.size() != y.size()) {
        Logger::ErrorLog(LOC, "Size mismatch: x.size() != y.size() in EvaluateSub.");
        return;
    }
    if (z.size() != x.size()) {
        z.resize(x.size());
    }
    for (size_t i = 0; i < x.size(); ++i) {
        z[i] = Mod(x[i] - y[i], bitsize_);
    }
}

void AdditiveSharing2P::EvaluateMult(const uint32_t party_id, oc::Channel &chl, const uint32_t &x, const uint32_t &y, uint32_t &z) {
    if (triple_index_ >= triples_.num_triples) {
        Logger::ErrorLog(LOC, "No more Beaver triples available.");
        return;
    }

    // Get the current Beaver triple
    const auto &triple = triples_.triples[triple_index_];
    ++triple_index_;

    // -------------------------------------------------------
    // 1) Prepare local differences: d = (x - a), e = (y - b)
    //    Then we reconstruct d, e across both parties.
    // -------------------------------------------------------

    // For convenience, define local arrays for d, e
    // We'll store them as:   de_0 = { d, e } on Party 0
    //                        de_1 = { d, e } on Party 1
    // Then we 'Reconst' them into a single array 'de' = { d_reconst, e_reconst }

    // We'll define them as std::array<T, 2> so that each of d, e is of type T.
    std::array<uint32_t, 2> de_0, de_1;

    if (party_id == 0) {
        de_0[0] = Mod(x - triple.a, bitsize_);    // d
        de_0[1] = Mod(y - triple.b, bitsize_);    // e
    } else {
        de_1[0] = Mod(x - triple.a, bitsize_);    // d
        de_1[1] = Mod(y - triple.b, bitsize_);    // e
    }

    // -------------------------------------------------------
    // 2) Reconstruct d, e across both parties
    //    We'll store it in 'de': { d_reconst, e_reconst }
    // -------------------------------------------------------
    std::array<uint32_t, 2> de;    // d_reconst = de[0], e_reconst = de[1]

    Reconst(party_id, chl, de_0, de_1, de);

    // -------------------------------------------------------
    // 3) Compute final product share z
    //    Beaver formula: z = a*e + b*d + c (+ d*e for party 0)
    // -------------------------------------------------------
    if (party_id == 0) {
        z = Mod((de[1] * triple.a) +        // a * e
                    (de[0] * triple.b) +    // b * d
                    triple.c +              // c
                    (de[0] * de[1]),        // d * e
                bitsize_);
    } else {
        z = Mod((de[1] * triple.a) +
                    (de[0] * triple.b) +
                    triple.c,
                bitsize_);
    }
}

void AdditiveSharing2P::EvaluateMult(const uint32_t party_id, oc::Channel &chl, const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, std::array<uint32_t, 2> &z) {
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
    // 1) Prepare local differences: d = (x - a), e = (y - b)
    //    Then we reconstruct d, e across both parties.
    // -------------------------------------------------------

    // For convenience, define local arrays for d, e
    // We'll store them as:   de_0 = { d0_0, e0_0, d1_0, e1_0 } on Party 0
    //                        de_1 = { d0_1, e0_1, d1_1, e1_1 } on Party 1
    // Then we 'Reconst' them into a single array 'de' = { d0_reconst, e0_reconst, d1_reconst, e1_reconst }

    // We'll define them as std::array<T, 4> so that each of d, e is of type T.
    std::array<uint32_t, 4> de_0, de_1;

    if (party_id == 0) {
        de_0[0] = Mod(x[0] - triple_0.a, bitsize_);    // d0
        de_0[1] = Mod(y[0] - triple_0.b, bitsize_);    // e0
        de_0[2] = Mod(x[1] - triple_1.a, bitsize_);    // d1
        de_0[3] = Mod(y[1] - triple_1.b, bitsize_);    // e1
    } else {
        de_1[0] = Mod(x[0] - triple_0.a, bitsize_);    // d0
        de_1[1] = Mod(y[0] - triple_0.b, bitsize_);    // e0
        de_1[2] = Mod(x[1] - triple_1.a, bitsize_);    // d1
        de_1[3] = Mod(y[1] - triple_1.b, bitsize_);    // e1
    }

    // -------------------------------------------------------
    // 2) Reconstruct d, e across both parties
    //    We'll store it in 'de': { d0_reconst, e0_reconst, d1_reconst, e1_reconst }
    // -------------------------------------------------------
    std::array<uint32_t, 4> de;    // d0_reconst = de[0], e0_reconst = de[1], d1_reconst = de[2], e1_reconst = de[3]
    Reconst(party_id, chl, de_0, de_1, de);

    // -------------------------------------------------------
    // 3) Compute final product share z
    //    Beaver formula: z = a*e + b*d + c (+ d*e for party 0)
    // -------------------------------------------------------
    if (party_id == 0) {
        z[0] = Mod((de[1] * triple_0.a) +        // a * e0
                       (de[0] * triple_0.b) +    // b * d0
                       triple_0.c +              // c0
                       (de[0] * de[1]),          // d0 * e0
                   bitsize_);
        z[1] = Mod((de[3] * triple_1.a) +        // a * e1
                       (de[2] * triple_1.b) +    // b * d1
                       triple_1.c +              // c1
                       (de[2] * de[3]),          // d1 * e1
                   bitsize_);
    } else {
        z[0] = Mod((de[1] * triple_0.a) +
                       (de[0] * triple_0.b) +
                       triple_0.c,
                   bitsize_);
        z[1] = Mod((de[3] * triple_1.a) +
                       (de[2] * triple_1.b) +
                       triple_1.c,
                   bitsize_);
    }
}

void AdditiveSharing2P::EvaluateSelect(const uint32_t party_id, oc::Channel &chl, const uint32_t &x, const uint32_t &y, const uint32_t &c, uint32_t &z) {
    // ----------------------------------------------------
    // 1) Compute y_sub_x = (y - x) mod bitsize
    // ----------------------------------------------------
    uint32_t y_sub_x;
    EvaluateSub(y, x, y_sub_x);

    // ----------------------------------------------------
    // 2) Compute c_mul_y_sub_x = c * (y - x) using Beaver triple
    //    This is a secure multiplication: EvaluateMult()
    // ----------------------------------------------------
    uint32_t c_mul_y_sub_x;
    EvaluateMult(party_id, chl, c, y_sub_x, c_mul_y_sub_x);

    // ----------------------------------------------------
    // 3) Finally, z = x + c_mul_y_sub_x
    // ----------------------------------------------------
    EvaluateAdd(x, c_mul_y_sub_x, z);
}

void AdditiveSharing2P::EvaluateSelect(const uint32_t party_id, oc::Channel &chl, const std::array<uint32_t, 2> &x, const std::array<uint32_t, 2> &y, const std::array<uint32_t, 2> &c, std::array<uint32_t, 2> &z) {
    std::array<uint32_t, 2> y_sub_x;
    EvaluateSub(y, x, y_sub_x);
    std::array<uint32_t, 2> c_mul_y_sub_x;
    EvaluateMult(party_id, chl, c, y_sub_x, c_mul_y_sub_x);
    EvaluateAdd(x, c_mul_y_sub_x, z);
}

uint32_t AdditiveSharing2P::GenerateRandomValue() const {
    return Mod(SecureRng::Rand32(), bitsize_);
}

void AdditiveSharing2P::PrintTriples() const {
    Logger::DebugLog(LOC, "Beaver triples:");
    triples_.DebugLog();
}

uint32_t AdditiveSharing2P::GetBitSize() const {
    return bitsize_;
}

uint32_t AdditiveSharing2P::GetCurrentTripleIndex() const {
    return triple_index_;
}

uint32_t AdditiveSharing2P::GetNumTriples() const {
    return triples_.num_triples;
}

uint32_t AdditiveSharing2P::GetRemainingTripleCount() const {
    return triples_.num_triples - triple_index_;
}

// ----------------------------------------------------
// Internal functions
// ----------------------------------------------------

void AdditiveSharing2P::GenerateBeaverTriples(const uint32_t num_triples, const uint32_t bitsize, BeaverTriples &triples) const {
    if (num_triples == 0) {
        Logger::ErrorLog(LOC, "Number of triples must be greater than 0.");
        return;
    }
    if (triples.num_triples != num_triples) {
        triples.triples.resize(num_triples);
    }
    for (uint32_t i = 0; i < num_triples; ++i) {
        uint32_t a         = Mod(prng_.get<uint32_t>(), bitsize);
        uint32_t b         = Mod(prng_.get<uint32_t>(), bitsize);
        uint32_t c         = Mod(a * b, bitsize);
        triples.triples[i] = {a, b, c};
    }
}

void AdditiveSharing2P::SaveTriplesShareToFile(const BeaverTriples &triples_0, const BeaverTriples &triples_1, const std::string &file_path) const {
    std::vector<uint8_t> buffer_0, buffer_1;
    triples_0.Serialize(buffer_0);
    triples_1.Serialize(buffer_1);

    FileIo io(".bt.bin");
    io.WriteToFileBinary(file_path + "_0", buffer_0);
    io.WriteToFileBinary(file_path + "_1", buffer_1);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Beaver triples saved successfully to " + file_path + io.GetExtension());
#endif
}

void AdditiveSharing2P::LoadTriplesShareFromFile(const uint32_t party_id, const std::string &file_path) {
    std::vector<uint8_t> buffer;
    FileIo               io(".bt.bin");
    io.ReadFromFileBinary(file_path + "_" + std::to_string(party_id), buffer);

    BeaverTriples triples;
    triples.Deserialize(buffer);
    triples_ = std::move(triples);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Beaver triples loaded successfully from " + file_path + io.GetExtension());
#endif
}

}    // namespace sharing
}    // namespace fsswm
