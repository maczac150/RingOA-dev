#ifndef SHARING_SHARE3_TYPES_H_
#define SHARING_SHARE3_TYPES_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace fsswm {
namespace sharing {

struct SharePair {
    std::array<uint32_t, 2> data;

    void DebugLog(const uint32_t party_id) const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

struct SharesPair {
    uint32_t                             num_shares;
    std::array<std::vector<uint32_t>, 2> data;

    void DebugLog(const uint32_t party_id) const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_SHARE3_TYPES_H_
