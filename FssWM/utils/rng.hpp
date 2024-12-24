#ifndef UTILS_RNG_H_
#define UTILS_RNG_H_

#include <array>
#include <openssl/rand.h>
#include <random>

namespace {

constexpr uint32_t kFixedSeed = 6;       // Fixed seed for random number generator
std::mt19937       mtrng(kFixedSeed);    // Mersenne Twister 19937 random number generator with fixed seed

}    // namespace

namespace utils {

using byte = uint8_t;    // Alias for a byte

class SecureRng {
public:
    // Generate a random 16-bit number.
    static inline uint16_t Rand16() {
        return Rand<uint16_t>();
    }

    // Generate a random 32-bit number.
    static inline uint32_t Rand32() {
        return Rand<uint32_t>();
    }

    // Generate a random 64-bit number.
    static inline uint64_t Rand64() {
        return Rand<uint64_t>();
    }

    // Generate a random boolean value.
    static inline bool RandBool() {
        return (Rand<uint16_t>() & 0x01) != 0;
    }

private:
    template <typename T>
    static T Rand() {

#ifdef RANDOM_SEED_FIXED
        std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
        return dist(mtrng);
#else
        std::array<byte, sizeof(T)> rand;
        int                         success = RAND_bytes(rand.data(), rand.size());
        if (!success) {
            std::perror("failed to create randomness");
            exit(EXIT_FAILURE);
        } else {
            T rand_num = rand[0];
            for (int i = 1; i < static_cast<int>(sizeof(T)); i++) {
                rand_num <<= 8;
                rand_num |= static_cast<T>(rand[i]);
            }
            return rand_num;
        }
#endif
    }
};

}    // namespace utils

#endif    // RNG_RNG_H_
