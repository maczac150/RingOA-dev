#include "beaver_triples.h"

#include <cstring>

#include "FssWM/utils/logger.h"

namespace fsswm {
namespace sharing {

BeaverTriple::BeaverTriple()
    : a(0), b(0), c(0) {
}

BeaverTriple::BeaverTriple(uint32_t a, uint32_t b, uint32_t c)
    : a(a), b(b), c(c) {
}

BeaverTriples::BeaverTriples(const uint32_t num_triples)
    : num_triples(num_triples), triples(num_triples) {
}

void BeaverTriples::DebugLog() const {
    for (uint32_t i = 0; i < num_triples; ++i) {
        Logger::DebugLog(LOC, "BTs[" + std::to_string(i) + "]: a = " + std::to_string(triples[i].a) +
                                         ", b = " + std::to_string(triples[i].b) +
                                         ", c = " + std::to_string(triples[i].c));
    }
}

bool BeaverTriples::Serialize(std::vector<uint8_t> &buffer) const {
    try {
        // Serialize num_triples (little-endian)
        uint32_t size = num_triples;
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t *>(&size), reinterpret_cast<uint8_t *>(&size) + sizeof(uint32_t));

        // Serialize each BeaverTriple
        for (const auto &triple : triples) {
            // Serialize a, b, c (little-endian)
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&triple.a), reinterpret_cast<const uint8_t *>(&triple.a) + sizeof(uint32_t));
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&triple.b), reinterpret_cast<const uint8_t *>(&triple.b) + sizeof(uint32_t));
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&triple.c), reinterpret_cast<const uint8_t *>(&triple.c) + sizeof(uint32_t));
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Successfully serialized BeaverTriples");
#endif
        return true;
    } catch (...) {
        Logger::ErrorLog(LOC, "Failed to serialize BeaverTriples");
        return false;
    }
}

bool BeaverTriples::Deserialize(const std::vector<uint8_t> &buffer) {
    try {
        // Ensure the buffer has at least the size of num_triples
        if (buffer.size() < sizeof(uint32_t)) {
            return false;
        }

        // Deserialize num_triples
        uint32_t size;
        std::memcpy(&size, buffer.data(), sizeof(uint32_t));

        // Calculate expected size
        size_t expected_size = sizeof(uint32_t) + size * 3 * sizeof(uint32_t);
        if (buffer.size() < expected_size) {
            Logger::ErrorLog(LOC, "Failed to deserialize BeaverTriples: buffer size is too small");
            return false;
        }

        num_triples = size;
        triples.resize(size);

        // Deserialize each BeaverTriple
        const uint8_t *ptr = buffer.data() + sizeof(uint32_t);
        for (auto &triple : triples) {
            std::memcpy(&triple.a, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            std::memcpy(&triple.b, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            std::memcpy(&triple.c, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
        }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Successfully deserialized BeaverTriples");
#endif
        return true;
    } catch (...) {
        Logger::ErrorLog(LOC, "Failed to deserialize BeaverTriples");
        return false;
    }
}

}    // namespace sharing
}    // namespace fsswm
