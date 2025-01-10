#ifndef SHARING_BEAVER_TRIPLES_H_
#define SHARING_BEAVER_TRIPLES_H_

#include <cstdint>
#include <vector>

namespace fsswm {
namespace sharing {

/**
 * @brief A struct to store a Beaver triple.
 */
struct BeaverTriple {
    uint32_t a;
    uint32_t b;
    uint32_t c;    // c = a * b

    BeaverTriple();
    BeaverTriple(uint32_t a, uint32_t b, uint32_t c);

    bool operator==(const BeaverTriple &rhs) const {
        return a == rhs.a && b == rhs.b && c == rhs.c;
    }

    bool operator!=(const BeaverTriple &rhs) const {
        return !(*this == rhs);
    }
};

struct BeaverTriples {
    uint32_t                  num_triples;
    std::vector<BeaverTriple> triples;

    BeaverTriples() = default;
    BeaverTriples(const uint32_t num_triples);

    BeaverTriples(const BeaverTriples &)            = delete;
    BeaverTriples &operator=(const BeaverTriples &) = delete;

    BeaverTriples(BeaverTriples &&) noexcept            = default;
    BeaverTriples &operator=(BeaverTriples &&) noexcept = default;

    bool operator==(const BeaverTriples &rhs) const {
        return num_triples == rhs.num_triples && triples == rhs.triples;
    }

    bool operator!=(const BeaverTriples &rhs) const {
        return !(*this == rhs);
    }

    void DebugLog() const;
    bool Serialize(std::vector<uint8_t> &buffer) const;
    bool Deserialize(const std::vector<uint8_t> &buffer);
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_BEAVER_TRIPLES_H_
