#ifndef SHARING_REP_SHARE_H_
#define SHARING_REP_SHARE_H_

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "FssWM/utils/block.h"
#include "FssWM/utils/to_string.h"

namespace fsswm {
namespace sharing {

// Generic share pair struct for uint64_t or block types
template <typename T>
struct RepShare {
    static_assert(
        std::is_same_v<T, uint64_t> || std::is_same_v<T, block>,
        "RepShare<T> supports only uint64_t or block");

    std::array<T, 2> data;

    // Default, copy, and move constructors/assignments
    RepShare()                                = default;
    RepShare(const RepShare &)                = default;
    RepShare(RepShare &&) noexcept            = default;
    RepShare &operator=(const RepShare &)     = default;
    RepShare &operator=(RepShare &&) noexcept = default;

    // Construct from two shares
    RepShare(T share0, T share1) {
        data[0] = share0;
        data[1] = share1;
    }

    // Construct from array
    RepShare(const std::array<T, 2> &other) : data(other) {
    }
    RepShare &operator=(const std::array<T, 2> &other) {
        data = other;
        return *this;
    }

    // Element access
    T &operator[](size_t idx) {
        return data[idx];
    }
    const T &operator[](size_t idx) const {
        return data[idx];
    }

    // Convert to string for debugging
    std::string ToString() const {
        std::ostringstream oss;
        if constexpr (std::is_same_v<T, block>) {
            oss << "(" << fsswm::Format(data[0]) << ", " << fsswm::Format(data[1]) << ")";
        } else {
            oss << "(" << data[0] << ", " << data[1] << ")";
        }
        return oss.str();
    }

    // Serialize into byte buffer (little-endian)
    void Serialize(std::vector<uint8_t> &buffer) const {
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data.data());
        buffer.insert(buffer.end(), ptr, ptr + 2 * sizeof(T));
    }

    // Deserialize from byte buffer (little-endian)
    void Deserialize(const std::vector<uint8_t> &buffer) {
        if (buffer.size() < 2 * sizeof(T))
            throw std::invalid_argument("RepShare::Deserialize: buffer too small");
        std::memcpy(data.data(), buffer.data(), 2 * sizeof(T));
    }
};

// Generic replicated share vector that holds two vectors of type T
// T must be trivially copyable for serialization
template <typename T>
struct RepShareVec {
    static_assert(
        std::is_same_v<T, uint64_t> || std::is_same_v<T, block>,
        "RepShareVec can only be used with uint64_t or block types");

    size_t                        num_shares;
    std::array<std::vector<T>, 2> data;

    RepShareVec()                                   = default;
    RepShareVec(const RepShareVec &)                = delete;
    RepShareVec(RepShareVec &&) noexcept            = delete;
    RepShareVec &operator=(const RepShareVec &)     = default;
    RepShareVec &operator=(RepShareVec &&) noexcept = default;

    explicit RepShareVec(size_t n)
        : num_shares(n) {
        data[0].resize(n);
        data[1].resize(n);
    }

    RepShareVec(std::vector<T> &&share_0, std::vector<T> &&share_1)
        : num_shares(share_0.size()) {
        if (share_0.size() != share_1.size()) {
            throw std::invalid_argument("Shares must have the same size");
        }
        data[0] = std::move(share_0);
        data[1] = std::move(share_1);
    }

    std::vector<T> &operator[](const size_t idx) {
        return data[idx];
    }
    const std::vector<T> &operator[](const size_t idx) const {
        return data[idx];
    }

    size_t Size() const {
        return num_shares;
    }

    RepShare<T> At(size_t idx) const {
        if (idx >= num_shares) {
            throw std::out_of_range("RepShareVec::At index out of range");
        }
        return RepShare<T>(data[0][idx], data[1][idx]);
    }

    void Set(size_t idx, const RepShare<T> &share) {
        if (idx >= num_shares) {
            throw std::out_of_range("RepShareVec::Set index out of range");
        }
        data[0][idx] = share[0];
        data[1][idx] = share[1];
    }

    std::string ToString(FormatType format = FormatType::kHex, std::string_view delim = " ", size_t max_size = kSizeMax) const {
        std::ostringstream oss;
        if constexpr (std::is_same_v<T, block>) {
            oss << "(" << fsswm::Format(data[0], format, delim, max_size) << ", " << fsswm::Format(data[1], format, delim, max_size) << ")";
        } else {
            oss << "(" << fsswm::ToString(data[0], delim, max_size) << ", " << fsswm::ToString(data[1], delim, max_size) << ")";
        }
        return oss.str();
    }

    void Serialize(std::vector<uint8_t> &buffer) const {
        // Serialize the number of shares
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&num_shares), reinterpret_cast<const uint8_t *>(&num_shares) + sizeof(num_shares));

        // Serialize the share data
        size_t bytes = sizeof(T) * num_shares;
        for (int i = 0; i < 2; ++i) {
            const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data[i].data());
            buffer.insert(buffer.end(), ptr, ptr + bytes);
        }
    }

    void Deserialize(const std::vector<uint8_t> &buffer) {
        size_t offset = 0;

        // Deserialize the number of shares
        std::memcpy(&num_shares, buffer.data() + offset, sizeof(num_shares));
        offset += sizeof(num_shares);

        // Resize the share data vectors
        data[0].resize(num_shares);
        data[1].resize(num_shares);

        // Deserialize the share data
        size_t bytes = sizeof(T) * num_shares;
        for (int i = 0; i < 2; ++i) {
            std::memcpy(data[i].data(), buffer.data() + offset, bytes);
            offset += bytes;
        }
    }
};

// Lightweight view for RepShareVec<T>, non-owning, read-only
template <typename T>
struct RepShareView {
    size_t             num_shares;
    std::span<const T> share0;
    std::span<const T> share1;

    explicit RepShareView(const RepShareVec<T> &rep_share)
        : num_shares(rep_share.num_shares),
          share0(rep_share.data[0]),
          share1(rep_share.data[1]) {
    }

    RepShareView(size_t             count,
                 std::span<const T> s0,
                 std::span<const T> s1) noexcept
        : num_shares(count),
          share0(s0),
          share1(s1) {
    }

    size_t Size() const {
        return num_shares;
    }

    RepShare<T> At(size_t idx) const {
        if (idx >= num_shares) {
            throw std::out_of_range("RepShareView::At index out of range");
        }
        return RepShare<T>(share0[idx], share1[idx]);
    }

    std::string ToString(FormatType format = FormatType::kHex, std::string_view delim = " ", size_t max_size = kSizeMax) const {
        std::ostringstream oss;
        if constexpr (std::is_same_v<T, block>) {
            oss << "(" << fsswm::Format(share0, format, delim, max_size) << ", " << fsswm::Format(share1, format, delim, max_size) << ")";
        } else {
            oss << "(" << fsswm::ToString(share0, delim, max_size) << ", " << fsswm::ToString(share1, delim, max_size) << ")";
        }
        return oss.str();
    }
};

template <typename T>
struct RepShareMat {
    size_t         rows, cols;
    RepShareVec<T> shares;    // Internally holds rows*cols × 2 shares

    static_assert(
        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, block>,
        "RepShareMat<T> supports only uint32_t, uint64_t, or block");

    RepShareMat(size_t rows_, size_t cols_)
        : rows(rows_), cols(cols_), shares(rows_ * cols_) {
    }

    RepShareMat(size_t rows_, size_t cols_, std::vector<T> &&share_0, std::vector<T> &&share_1)
        : rows(rows_), cols(cols_), shares(0) {
        size_t n = rows_ * cols_;
        if (share_0.size() != n || share_1.size() != n) {
            throw std::invalid_argument("RepShareMat: flat vector size mismatch");
        }
        // Move into internal shares
        shares.data[0]    = std::move(share_0);
        shares.data[1]    = std::move(share_1);
        shares.num_shares = n;
    }

    RepShareMat()                                   = default;
    RepShareMat(const RepShareMat &)                = delete;
    RepShareMat(RepShareMat &&) noexcept            = delete;
    RepShareMat &operator=(const RepShareMat &)     = default;
    RepShareMat &operator=(RepShareMat &&) noexcept = default;

    std::vector<T> &operator[](const size_t idx) {
        return shares.data[idx];
    }
    const std::vector<T> &operator[](const size_t idx) const {
        return shares.data[idx];
    }

    RepShareView<T> RowView(size_t i) const {
        if (i >= rows) {
            throw std::out_of_range("Row index out of range");
        }
        size_t             offset = i * cols;
        std::span<const T> s0(shares.data[0].data() + offset, cols);
        std::span<const T> s1(shares.data[1].data() + offset, cols);
        return RepShareView<T>(cols, s0, s1);
    }

    RepShare<T> At(size_t i, size_t j) const {
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Index out of range");
        }
        return shares.At(i * cols + j);
    }

    // String representation up to optional row/col limits
    std::string ToStringMatrix(FormatType       format   = FormatType::kHex,
                               std::string_view row_pref = "[",
                               std::string_view row_suff = "]",
                               std::string_view col_del  = " ",
                               std::string_view row_del  = ",",
                               size_t           max_size = kSizeMax) const {
        std::ostringstream oss;
        if constexpr (std::is_same_v<T, block>) {
            oss << "(" << fsswm::FormatMatrix(shares.data[0], rows, cols, format, row_pref, row_suff, col_del, row_del, max_size) << ", "
                << fsswm::FormatMatrix(shares.data[1], rows, cols, format, row_pref, row_suff, col_del, row_del, max_size) << ")";
            return oss.str();
        } else {
            oss << "(" << fsswm::ToStringMatrix(shares.data[0], rows, cols, row_pref, row_suff, col_del, row_del, max_size) << ", "
                << fsswm::ToStringMatrix(shares.data[1], rows, cols, row_pref, row_suff, col_del, row_del, max_size) << ")";
        }
        return oss.str();
    }

    void Serialize(std::vector<uint8_t> &buffer) const {
        // Serialize dimensions
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&rows), reinterpret_cast<const uint8_t *>(&rows) + sizeof(rows));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t *>(&cols), reinterpret_cast<const uint8_t *>(&cols) + sizeof(cols));

        // Serialize the share data
        shares.Serialize(buffer);
    }

    void Deserialize(const std::vector<uint8_t> &buffer) {
        size_t offset = 0;

        // Deserialize dimensions
        std::memcpy(&rows, buffer.data() + offset, sizeof(rows));
        offset += sizeof(rows);
        std::memcpy(&cols, buffer.data() + offset, sizeof(cols));
        offset += sizeof(cols);

        // Resize internal shares
        shares.num_shares = rows * cols;
        shares.data[0].resize(shares.num_shares);
        shares.data[1].resize(shares.num_shares);

        // Deserialize the share data
        shares.Deserialize(std::vector<uint8_t>(buffer.begin() + offset, buffer.end()));
    }
};

}    // namespace sharing
}    // namespace fsswm

#endif    // SHARING_REP_SHARE_H_
