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
using UIntVec                = std::vector<uint32_t>;
using UIntMat                = std::vector<std::vector<uint32_t>>;

struct Channels {
    uint32_t    party_id;
    oc::Channel prev;
    oc::Channel next;

    Channels(const uint32_t party_id, oc::Channel &prev, oc::Channel &next)
        : party_id(party_id), prev(prev), next(next) {
    }

    uint64_t GetStats() {
        return prev.getTotalDataSent() + next.getTotalDataSent();
    }

    void ResetStats() {
        prev.resetStats();
        next.resetStats();
    }
};

struct RepShare {
    std::array<uint32_t, 2> data;

    RepShare()                                = default;
    RepShare(const RepShare &)                = default;
    RepShare(RepShare &&) noexcept            = default;
    RepShare &operator=(const RepShare &)     = default;
    RepShare &operator=(RepShare &&) noexcept = default;

    RepShare(const uint32_t share_0, const uint32_t share_1) {
        data[0] = share_0;
        data[1] = share_1;
    }

    RepShare(const std::array<uint32_t, 2> &other) {
        data = other;
    }

    RepShare &operator=(const std::array<uint32_t, 2> &other) {
        data = other;
        return *this;
    }

    uint32_t &operator[](const size_t idx) {
        return data[idx];
    }
    const uint32_t &operator[](const size_t idx) const {
        return data[idx];
    }

    void DebugLog(const uint32_t party_id, const std::string &prefix = "x") const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

struct RepShareVec {
    uint32_t               num_shares;
    std::array<UIntVec, 2> data;

    RepShareVec()                                   = default;
    RepShareVec(const RepShareVec &)                = default;
    RepShareVec(RepShareVec &&) noexcept            = default;
    RepShareVec &operator=(const RepShareVec &)     = default;
    RepShareVec &operator=(RepShareVec &&) noexcept = default;

    RepShareVec(uint32_t num_shares_)
        : num_shares(num_shares_) {
        data[0].resize(num_shares_);
        data[1].resize(num_shares_);
    }

    RepShareVec(const UIntVec &share_0, const UIntVec &share_1)
        : num_shares(share_0.size()) {
        data[0] = share_0;
        data[1] = share_1;
    }

    RepShareVec(const UIntVec &&share_0, const UIntVec &&share_1)
        : num_shares(share_0.size()) {
        data[0] = std::move(share_0);
        data[1] = std::move(share_1);
    }

    UIntVec &operator[](const size_t idx) {
        return data[idx];
    }
    const UIntVec &operator[](const size_t idx) const {
        return data[idx];
    }

    RepShare At(const size_t idx) const {
        if (idx >= num_shares) {
            throw std::out_of_range("Index out of range");
        }
        return RepShare{data[0][idx], data[1][idx]};
    }

    void Set(const size_t idx, const RepShare &share) {
        if (idx >= num_shares) {
            throw std::out_of_range("Index out of range");
        }
        data[0][idx] = share[0];
        data[1][idx] = share[1];
    }

    void DebugLog(const uint32_t party_id, const std::string &prefix = "x") const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

struct RepShareVecView {
    const UIntVec &share0;
    const UIntVec &share1;

    uint32_t num_shares() const {
        return static_cast<uint32_t>(share0.size());
    }

    const UIntVec &operator[](size_t index) const {
        if (index == 0) {
            return share0;
        } else if (index == 1) {
            return share1;
        } else {
            throw std::out_of_range("Index out of range");
        }
    }

    RepShare At(const size_t idx) const {
        if (idx >= num_shares()) {
            throw std::out_of_range("Index out of range");
        }
        return RepShare{share0[idx], share1[idx]};
    }

    void DebugLog(const uint32_t party_id, const std::string &prefix = "x") const;
};

struct RepShareMat {
    uint32_t               rows;
    uint32_t               cols;
    std::array<UIntMat, 2> data;

    RepShareMat()                                   = default;
    RepShareMat(const RepShareMat &)                = default;
    RepShareMat(RepShareMat &&) noexcept            = default;
    RepShareMat &operator=(const RepShareMat &)     = default;
    RepShareMat &operator=(RepShareMat &&) noexcept = default;

    RepShareMat(const UIntMat &share_0, const UIntMat &share_1)
        : rows(share_0.size()), cols(share_0.empty() ? 0 : share_0[0].size()) {
        data[0] = share_0;
        data[1] = share_1;
    }

    RepShareMat(const UIntMat &&share_0, const UIntMat &&share_1)
        : rows(share_0.size()), cols(share_0.empty() ? 0 : share_0[0].size()) {
        data[0] = std::move(share_0);
        data[1] = std::move(share_1);
    }

    UIntMat &operator[](const size_t idx) {
        return data[idx];
    }
    const UIntMat &operator[](const size_t idx) const {
        return data[idx];
    }

    RepShareVecView RowView(const size_t row) const {
        if (row >= rows) {
            throw std::out_of_range("Row index out of range");
        }
        return RepShareVecView{data[0][row], data[1][row]};
    }

    void DebugLog(const uint32_t party_id, const std::string &prefix = "x") const;
    void Serialize(std::vector<uint8_t> &buffer) const;
    void Deserialize(const std::vector<uint8_t> &buffer);
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_SHARE3_TYPES_H_
