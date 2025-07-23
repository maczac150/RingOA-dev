#ifndef SHARING_BEAVER_TRIPLES_H_
#define SHARING_BEAVER_TRIPLES_H_

#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace fsswm {
namespace sharing {

/**
 * @brief A struct to store a Beaver triple.
 */
struct BeaverTriple {
    uint64_t a;
    uint64_t b;
    uint64_t c;    // c = a * b

    BeaverTriple()
        : a(0), b(0), c(0) {
    }
    BeaverTriple(uint64_t a_, uint64_t b_, uint64_t c_)
        : a(a_), b(b_), c(c_) {
    }

    bool operator==(const BeaverTriple &rhs) const {
        return a == rhs.a && b == rhs.b && c == rhs.c;
    }

    bool operator!=(const BeaverTriple &rhs) const {
        return !(*this == rhs);
    }
};

struct BeaverTriples {
    size_t                    num_triples;
    std::vector<BeaverTriple> triples;

    BeaverTriples() = default;
    BeaverTriples(const size_t n)
        : num_triples(n), triples(n) {
    }

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

    std::string ToString(size_t limit = 0, std::string delimiter = ", ") const {
        if (limit == 0 || limit > num_triples) {
            limit = num_triples;
        }
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < limit; ++i) {
            oss << "(" << triples[i].a << "," << triples[i].b << "," << triples[i].c << ")";
            if (i < limit - 1) {
                oss << delimiter;
            }
        }
        if (limit < num_triples) {
            oss << "...";
        }
        oss << "]";
        return oss.str();
    }

    void Serialize(std::vector<uint8_t> &buffer) const {
        // Serialize number of triples
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_triples), reinterpret_cast<const uint8_t *>(&num_triples) + sizeof(num_triples));

        // Serialize each triple
        for (const auto &triple : triples) {
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&triple.a), reinterpret_cast<const uint8_t *>(&triple.a) + sizeof(triple.a));
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&triple.b), reinterpret_cast<const uint8_t *>(&triple.b) + sizeof(triple.b));
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&triple.c), reinterpret_cast<const uint8_t *>(&triple.c) + sizeof(triple.c));
        }
    }

    void Deserialize(const std::vector<uint8_t> &buffer) {
        size_t offset = 0;
        // Deserialize number of triples
        std::memcpy(&num_triples, buffer.data() + offset, sizeof(num_triples));
        offset += sizeof(num_triples);

        // Calculate expected size
        size_t expected_size = sizeof(uint64_t) + num_triples * 3 * sizeof(uint64_t);
        if (buffer.size() < expected_size) {
            throw std::runtime_error("Buffer size is too small for deserialization");
        }

        // Resize triples vector
        triples.resize(num_triples);

        // Deserialize each BeaverTriple
        for (size_t i = 0; i < num_triples; ++i) {
            std::memcpy(&triples[i].a, buffer.data() + offset, sizeof(triples[i].a));
            offset += sizeof(triples[i].a);
            std::memcpy(&triples[i].b, buffer.data() + offset, sizeof(triples[i].b));
            offset += sizeof(triples[i].b);
            std::memcpy(&triples[i].c, buffer.data() + offset, sizeof(triples[i].c));
            offset += sizeof(triples[i].c);
        }
    }
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_BEAVER_TRIPLES_H_
