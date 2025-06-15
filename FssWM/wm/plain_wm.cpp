#include "plain_wm.h"

#include <algorithm>
#include <sdsl/csa_wt.hpp>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/to_string.h"
#include "FssWM/utils/utils.h"

namespace fsswm {
namespace wm {

CharMapper::CharMapper(CharType type)
    : type_(type) {
    Initialize(type);
}

void CharMapper::Initialize(CharType type) {
    char2id_.clear();
    id2char_.clear();

    switch (type) {
        case CharType::DNA:
            sigma_   = 3;
            char2id_ = {{'$', 0}, {'A', 1}, {'C', 2}, {'G', 3}, {'T', 4}};
            break;
        case CharType::PROTEIN:
            sigma_   = 5;
            char2id_ = {
                {'$', 0},
                {'A', 1},
                {'C', 2},
                {'D', 3},
                {'E', 4},
                {'F', 5},
                {'G', 6},
                {'H', 7},
                {'I', 8},
                {'K', 9},
                {'L', 10},
                {'M', 11},
                {'N', 12},
                {'P', 13},
                {'Q', 14},
                {'R', 15},
                {'S', 16},
                {'T', 17},
                {'V', 18},
                {'W', 19},
                {'Y', 20}};
            break;
        default:
            Logger::ErrorLog(LOC, "Unknown character type");
            break;
    }

    id2char_.resize(char2id_.size());
    for (const auto &[ch, id] : char2id_) {
        id2char_[id] = ch;
    }
}

size_t CharMapper::GetSigma() const {
    return sigma_;
}

CharType CharMapper::GetType() const {
    return type_;
}

bool CharMapper::IsValidChar(char c) const {
    return char2id_.find(c) != char2id_.end();
}

std::vector<uint64_t> CharMapper::ToIds(const std::string &s) const {
    std::vector<uint64_t> result;
    result.reserve(s.size());
    for (char c : s) {
        result.push_back(ToId(c));
    }
    return result;
}

uint64_t CharMapper::ToId(char c) const {
    auto it = char2id_.find(c);
    if (it == char2id_.end()) {
        Logger::ErrorLog(LOC, "Character '" + std::string(1, c) + "' not found in alphabet");
    }
    return it->second;
}

std::string CharMapper::ToString(const std::vector<uint64_t> &v) const {
    std::string result;
    result.reserve(v.size());
    for (uint64_t id : v) {
        result += id2char_.at(id);
    }
    return result;
}

const std::unordered_map<char, uint64_t> &CharMapper::GetMap() const {
    return char2id_;
}

std::string CharMapper::MapToString() const {
    std::string result;
    for (const auto &[ch, id] : char2id_) {
        result += ch;
        result += ":";
        result += std::to_string(id);
        result += " ";
    }
    return result;
}

WaveletMatrix::WaveletMatrix(const std::string &data, const CharType type)
    : length_(0), sigma_(0), mapper_(type) {
    data_  = mapper_.ToIds(data);
    sigma_ = mapper_.GetSigma();
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma_));
    Logger::DebugLog(LOC, "Mapping: " + mapper_.MapToString());
    Logger::DebugLog(LOC, "Data: " + ToString(data_));
    Logger::DebugLog(LOC, "Length: " + ToString(data.size()));
#endif
    Build(data_);
}

WaveletMatrix::WaveletMatrix(const std::vector<uint64_t> &data, const size_t sigma)
    : length_(0), sigma_(sigma) {
    data_ = data;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Sigma: " + ToString(sigma_));
    Logger::DebugLog(LOC, "Data: " + ToString(data_));
    Logger::DebugLog(LOC, "Length: " + ToString(data.size()));
#endif
    Build(data_);
}

size_t WaveletMatrix::GetLength() const {
    return length_;
}

size_t WaveletMatrix::GetSigma() const {
    return sigma_;
}

const CharMapper &WaveletMatrix::GetMapper() const {
    return mapper_;
}

std::string WaveletMatrix::GetMapString() const {
    return mapper_.MapToString();
}

const std::vector<uint64_t> &WaveletMatrix::GetData() const {
    return data_;
}

const std::vector<uint64_t> &WaveletMatrix::GetRank0Tables() const {
    return rank0_tables_;
}

const std::vector<uint64_t> &WaveletMatrix::GetRank1Tables() const {
    return rank1_tables_;
}

void WaveletMatrix::PrintRank0Tables() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    const size_t stride = length_ + 1;
    for (size_t bit = 0; bit < sigma_; ++bit) {
        size_t                    off = bit * stride;
        std::span<const uint64_t> tbl(&rank0_tables_[off], stride);
        Logger::DebugLog(
            LOC,
            "Rank0 Table[" + ToString(bit) + "]: " +
                ToString(tbl));
    }
#endif
}

void WaveletMatrix::PrintRank1Tables() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    const size_t stride = length_ + 1;
    for (size_t bit = 0; bit < sigma_; ++bit) {
        size_t                    off = bit * stride;
        std::span<const uint64_t> tbl(&rank1_tables_[off], stride);
        Logger::DebugLog(
            LOC,
            "Rank1 Table[" + ToString(bit) + "]: " +
                ToString(tbl));
    }
#endif
}

uint64_t WaveletMatrix::RankCF(uint64_t c, size_t position) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "RankCF(" + ToString(c) + ", " + ToString(position) + ")");
#endif

    // for level ℓ, the offset into each flat table is ℓ*(length_+1)
    const size_t stride = length_ + 1;

    // Traverse from the LSB to the MSB
    for (size_t bit = 0; bit < sigma_; ++bit) {
        size_t off = bit * stride;
        bool   b   = (c >> bit) & 1;
        // if zero‐bit, jump to rank0; else jump to rank1
        position = b ? rank1_tables_[off + position]
                     : rank0_tables_[off + position];
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "(" + ToString(bit) + ") bit: " + ToString(b) + " -> RankCF: " + ToString(position));
#endif
    }
    return position;
}

uint64_t WaveletMatrix::kthSmallest(size_t l, size_t r, size_t k) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "kthSmallest(" + ToString(l) + ", " + ToString(r) + ", " + ToString(k) + ")");
#endif

    uint64_t     result = 0;
    size_t       left   = l;
    size_t       right  = r;
    const size_t stride = length_ + 1;

    // Traverse from most significant bit (sigma_-1) down to 0
    for (size_t lvl = sigma_; lvl > 0; --lvl) {
        size_t bit = lvl - 1;
        size_t off = bit * stride;

        // count zeros in [left, right)
        size_t z_left     = rank0_tables_[off + left];
        size_t z_right    = rank0_tables_[off + right];
        size_t zero_count = z_right - z_left;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Level " + ToString(lvl) + " (bit: " + ToString(bit) + ") - "
                                                                                     "z_left: " +
                                  ToString(z_left) + ", z_right: " + ToString(z_right) +
                                  ", zero_count: " + ToString(zero_count));
#endif

        if (k < zero_count) {
            // k-th lies in the 0-bucket
            left  = z_left;
            right = z_right;
        } else {
            // k-th lies in the 1-bucket
            k -= zero_count;

            // total number of zeros across the entire level
            size_t total_zeros = rank0_tables_[off + length_ - 1];
            // number of ones before left/right
            size_t o_left  = (left - z_left);
            size_t o_right = (right - z_right);

            left  = total_zeros + o_left;
            right = total_zeros + o_right;

            // set this bit in the result
            result |= (1ULL << bit);
        }
    }

    return result;
}

void WaveletMatrix::Build(const std::vector<uint64_t> &data) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "WaveletMatrix Build...");
#endif
    length_ = data.size();
    if (length_ == 0) {
        rank0_tables_.clear();
        return;
    }

    const size_t stride = length_ + 1;
    rank0_tables_.assign(sigma_ * stride, 0);
    rank1_tables_.assign(sigma_ * stride, 0);

    std::vector<uint64_t> current = data;
    std::vector<uint64_t> zero_bucket(length_), one_bucket(length_);

    // Build from the LSB to the MSB
    for (size_t bit = 0; bit < sigma_; ++bit) {
        size_t zeros = 0, ones = 0;
        size_t off = bit * stride;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        std::string bit_str = "";
#endif

        // 1) build rank0 counts and separate into buckets
        for (size_t i = 0; i < length_; ++i) {
            bool is_one = (current[i] >> bit) & 1U;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            bit_str += ToString(is_one);
#endif
            if (is_one) {
                one_bucket[ones++] = current[i];
            } else {
                zero_bucket[zeros++] = current[i];
                ++rank0_tables_[off + i + 1];    // Increment the count of zeros
            }
            rank0_tables_[off + i + 1] += rank0_tables_[off + i];    // Carry forward the count of zeros
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Bit Vector  [" + ToString(bit) + "]: " + bit_str +
                                  " (0: " + ToString(zeros) + ", 1: " + ToString(ones) + ")");
#endif

        // 2) build rank1 as (i – rank0) + totalZeros
        uint64_t total_zeros = rank0_tables_[off + length_];
        for (size_t i = 0; i < length_ + 1; ++i) {
            rank1_tables_[off + i] = (i - rank0_tables_[off + i]) + total_zeros;
        }

        std::copy(zero_bucket.begin(), zero_bucket.begin() + zeros, current.begin());
        std::copy(one_bucket.begin(), one_bucket.begin() + ones, current.begin() + zeros);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    PrintRank0Tables();
    PrintRank1Tables();
    Logger::DebugLog(LOC, "WaveletMatrix Build - Done");
#endif
}

FMIndex::FMIndex(const std::string &text, const CharType type) {
    // 1) Set text
    text_ = text;
    std::reverse(text_.begin(), text_.end());

    // 2) Build BWT from text
    BuildBwt();

    // 3) Convert bwt_str_ to integers
    wm_ = WaveletMatrix(bwt_str_, type);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, kDash);
    Logger::DebugLog(LOC, "Alphabet size   : " + ToString(wm_.GetSigma()));
    Logger::DebugLog(LOC, "Mapping         : " + wm_.GetMapString());
    Logger::DebugLog(LOC, "Text            : " + text_);
    Logger::DebugLog(LOC, "BWT             : " + bwt_str_);
    Logger::DebugLog(LOC, "BWT as integers : " + ToString(wm_.GetData()));
    wm_.PrintRank0Tables();
    wm_.PrintRank1Tables();
    Logger::DebugLog(LOC, kDash);
#endif
}

const WaveletMatrix &FMIndex::GetWaveletMatrix() const {
    return wm_;
}

const std::vector<uint64_t> &FMIndex::GetRank0Tables() const {
    return wm_.GetRank0Tables();
}

const std::vector<uint64_t> &FMIndex::GetRank1Tables() const {
    return wm_.GetRank1Tables();
}

std::vector<uint64_t> FMIndex::ConvertToBitMatrix(const std::string &query) const {
    // 1) string -> ID
    std::vector<uint64_t> nums = wm_.GetMapper().ToIds(query);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query: " + query);
    Logger::DebugLog(LOC, "Query as numbers: " + ToString(nums));
#endif

    // 2) Convert to bits
    uint64_t              sigma = wm_.GetSigma();
    std::vector<uint64_t> bits(nums.size() * sigma);
    for (size_t i = 0; i < nums.size(); ++i) {
        uint64_t val = nums[i];
        for (size_t b = 0; b < sigma; ++b) {
            bits[i * sigma + b] = (val >> b) & 1U;
        }
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (size_t i = 0; i < bits.size(); ++i) {
        Logger::DebugLog(LOC, "bit_row[" + ToString(i) + "]: " + ToString(bits[i]));
    }
#endif
    return bits;
}

void FMIndex::BuildBwt() {
    // Construct the suffix array using the SDSL library
    sdsl::csa_wt<> csa;
    sdsl::construct_im(csa, text_, 1);
    // Convert the BWT to a string
    for (size_t i = 0; i < text_.size() + 1; ++i) {
        if (csa.bwt[i]) {
            bwt_str_ += csa.bwt[i];
        } else {
            bwt_str_ += '$';
        }
    }
}

void FMIndex::BackwardSearch(char c, uint64_t &left, uint64_t &right) const {
    if (!wm_.GetMapper().IsValidChar(c)) {
        Logger::ErrorLog(LOC, "Invalid character '" + std::string(1, c) + "' in BackwardSearch");
    }
    uint64_t cid = wm_.GetMapper().ToId(c);
    left         = wm_.RankCF(cid, left);
    right        = wm_.RankCF(cid, right);
}

uint64_t FMIndex::ComputeLPMfromWM(const std::string &query) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "lpm_len(" + query + ")");
#endif

    // Backward search for last character
    uint64_t              left  = 0;
    uint64_t              right = static_cast<uint64_t>(bwt_str_.size());
    std::vector<uint64_t> intervals;
    // Traverse query from end to front
    for (size_t i = 0; i < query.size(); ++i) {
        char c = query[i];
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "(char " + std::string(1, c) + ") (l, r) == (" + ToString(left) + ", " + ToString(right) + ")");
#endif
        BackwardSearch(c, left, right);
        intervals.push_back(right - left);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "(l, r) == (" + ToString(left) + ", " + ToString(right) + ")");
    Logger::DebugLog(LOC, "Intervals: " + ToString(intervals));
#endif
    uint64_t lpm_len = 0;
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (intervals[i] == 0) {
            break;
        }
        lpm_len++;
    }
    return lpm_len;
}

uint64_t FMIndex::ComputeLPMfromBWT(const std::string &query) const {
    const std::string &bwt = bwt_str_;
    const size_t       n   = bwt.size();

    // Step 1: count character frequency
    std::unordered_map<char, int> char_count;
    for (char c : bwt) {
        char_count[c]++;
    }

    // Step 2: build F[c] = offset (no need for range)
    std::map<char, int> F;
    int                 offset = 0;
    for (const auto &[c, count] : std::map<char, int>(char_count.begin(), char_count.end())) {
        F[c] = offset;
        offset += count;
    }
    // #if LOG_LEVEL >= LOG_LEVEL_DEBUG
    //     Logger::DebugLog(LOC, "F[c] = offset:");
    //     for (const auto &[c, offset] : F) {
    //         Logger::DebugLog(LOC, "  '" + std::string(1, c) + "' : " + ToString(offset));
    //     }
    // #endif

    // Step 3: backward search with rank() + offset
    int                   f       = 0;
    int                   g       = static_cast<int>(n);
    uint64_t              lpm_len = 0;
    std::vector<uint64_t> intervals;

    for (size_t qi = 0; qi < query.size(); ++qi) {
        char        c     = query[qi];
        std::string bwt_f = bwt.substr(0, f);
        std::string bwt_g = bwt.substr(0, g);

        int rank_f = static_cast<int>(std::count(bwt_f.begin(), bwt_f.end(), c));
        int rank_g = static_cast<int>(std::count(bwt_g.begin(), bwt_g.end(), c));
        int offset = F[c];

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "(char: " + std::string(1, c) + ") (l, r) == (" +
                                  ToString(f) + ", " + ToString(g) + ")");
        Logger::DebugLog(LOC, "(char: " + std::string(1, c) + ") l = offset(" + ToString(offset) +
                                  ") + rank(" + std::string(1, c) + ", " + ToString(f) + ")(" +
                                  ToString(rank_f) + ")");
        Logger::DebugLog(LOC, "(char: " + std::string(1, c) + ") r = offset(" + ToString(offset) +
                                  ") + rank(" + std::string(1, c) + ", " + ToString(g) + ")(" +
                                  ToString(rank_g) + ")");
#endif

        f = offset + rank_f;
        g = offset + rank_g;

        if (f < g) {
            lpm_len++;
        }
        intervals.push_back(g - f);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "(l, r) == (" + ToString(f) + ", " + ToString(g) + ")");
    Logger::DebugLog(LOC, "Intervals: " + ToString(intervals));
    Logger::DebugLog(LOC, "LPM length (without WM): " + ToString(lpm_len));
#endif

    return lpm_len;
}

}    // namespace wm
}    // namespace fsswm
