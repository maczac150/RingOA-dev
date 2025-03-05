#ifndef SHARING_SHARE3_TYPES_H_
#define SHARING_SHARE3_TYPES_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "cryptoTools/Network/Channel.h"

namespace fsswm {
namespace sharing {

constexpr size_t kNumParties = 3;

struct Channels {
    uint32_t    party_id;
    oc::Channel prev;
    oc::Channel next;

    Channels(const uint32_t party_id, oc::Channel &prev, oc::Channel &next)
        : party_id(party_id), prev(prev), next(next) {
    }
};

struct SharePair {
    std::array<uint32_t, 2> data;

    SharePair() = default;
    SharePair(const uint32_t share_0, const uint32_t share_1);

    void DebugLog(const uint32_t party_id, const std::string &prefix = "x") const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

struct SharesPair {
    uint32_t                             num_shares;
    std::array<std::vector<uint32_t>, 2> data;

    void DebugLog(const uint32_t party_id, const std::string &prefix = "x") const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_SHARE3_TYPES_H_
