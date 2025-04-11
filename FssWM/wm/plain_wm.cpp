#include "plain_wm.h"

#include <algorithm>
#include <sdsl/csa_wt.hpp>

#include "FssWM/utils/logger.h"
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

std::vector<uint32_t> CharMapper::ToIds(const std::string &s) const {
    std::vector<uint32_t> result;
    result.reserve(s.size());
    for (char c : s) {
        result.push_back(ToId(c));
    }
    return result;
}

uint32_t CharMapper::ToId(char c) const {
    auto it = char2id_.find(c);
    if (it == char2id_.end()) {
        Logger::ErrorLog(LOC, "Character '" + std::string(1, c) + "' not found in alphabet");
    }
    return it->second;
}

std::string CharMapper::ToString(const std::vector<uint32_t> &v) const {
    std::string result;
    result.reserve(v.size());
    for (uint32_t id : v) {
        result += id2char_.at(id);
    }
    return result;
}

const std::unordered_map<char, uint32_t> &CharMapper::GetMap() const {
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
    : mapper_(type) {
    data_  = mapper_.ToIds(data);
    sigma_ = mapper_.GetSigma();
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Sigma: " + std::to_string(sigma_));
    Logger::DebugLog(LOC, "Mapping: " + mapper_.MapToString());
    Logger::DebugLog(LOC, "Data: " + ToString(data));
    Logger::DebugLog(LOC, "Length: " + std::to_string(data.size()));
#endif
    Build(data_);
}

WaveletMatrix::WaveletMatrix(const std::vector<uint32_t> &data, const size_t sigma)
    : sigma_(sigma) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Sigma: " + std::to_string(sigma_));
    Logger::DebugLog(LOC, "Data: " + ToString(data));
    Logger::DebugLog(LOC, "Length: " + std::to_string(data.size()));
#endif
    Build(data);
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

const std::vector<uint32_t> &WaveletMatrix::GetData() const {
    return data_;
}

const std::vector<std::vector<uint32_t>> &WaveletMatrix::GetRank0Tables() const {
    return rank0_tables_;
}

const std::vector<std::vector<uint32_t>> &WaveletMatrix::GetRank1Tables() const {
    return rank1_tables_;
}

void WaveletMatrix::PrintRank0Tables() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (size_t bit = 0; bit < sigma_; ++bit) {
        Logger::DebugLog(LOC, "Rank0 Tables[" + std::to_string(bit) + "]: " + ToString(rank0_tables_[bit]));
    }
#endif
}

void WaveletMatrix::PrintRank1Tables() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (size_t bit = 0; bit < sigma_; ++bit) {
        Logger::DebugLog(LOC, "Rank1 Tables[" + std::to_string(bit) + "]: " + ToString(rank1_tables_[bit]));
    }
#endif
}

uint32_t WaveletMatrix::RankCF(uint32_t c, size_t position) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "RankCF(" + std::to_string(c) + ", " + std::to_string(position) + ")");
#endif

    // Traverse from the LSB to the MSB
    for (size_t bit = 0; bit < sigma_; ++bit) {
        // Which bit are we looking at?
        bool bit_val = (c >> bit) & 1U;
        // Number of 1 bits in [0, position) at bit-level = (BITS-1-bit)
        uint32_t zeros_count = rank0_tables_[bit][position];
        if (!bit_val) {
            position = zeros_count;
        } else {
            position = (position - zeros_count) + rank0_tables_[bit][length_];
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "(" + std::to_string(bit) + ") bit: " + std::to_string(bit_val) + " -> RankCF: " + std::to_string(position));
#endif
    }
    return position;
}

void WaveletMatrix::Build(const std::vector<uint32_t> &data) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "WaveletMatrix Build...");
#endif
    length_ = data.size();
    if (length_ == 0) {
        for (size_t b = 0; b < sigma_; ++b) {
            rank0_tables_[b].clear();
        }
        return;
    }

    // rank0_tables_[b] のサイズを length_+1 に一括確保し、[0]～[length_] を使う
    rank0_tables_.resize(sigma_);
    for (size_t b = 0; b < sigma_; ++b) {
        // size = length_+1 (prefix sums: rank0_tables_[b][0] = 0)
        rank0_tables_[b].resize(length_ + 1, 0);
    }

    // current_data に data をコピー
    // 大規模データなら .assign() or .insert() よりも resize + std::copy が高速な場合も多い
    std::vector<uint32_t> current_data(length_);
    std::copy(data.begin(), data.end(), current_data.begin());

    // バケット配列 (再利用)
    // あらかじめ最大サイズ length_ にしておき、push_back ではなくインデックスで操作する
    std::vector<uint32_t> zero_bucket(length_);
    std::vector<uint32_t> one_bucket(length_);

    // Build from the LSB to the MSB
    for (size_t bit = 0; bit < sigma_; ++bit) {
        size_t   zero_count     = 0;
        size_t   one_count      = 0;
        uint32_t run_zero_count = 0;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        std::string bit_str = "";
#endif

        for (size_t i = 0; i < length_; ++i) {
            bool bit_val = (current_data[i] >> bit) & 1U;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            bit_str += std::to_string(bit_val);

#endif

            if (bit_val) {
                one_bucket[one_count++] = current_data[i];
            } else {
                zero_bucket[zero_count++] = current_data[i];
                run_zero_count++;
            }
            // rank0_tables_[bit][i+1] = これまでに出現した 0 の数
            rank0_tables_[bit][i + 1] = run_zero_count;
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "Bit Vector  [" + std::to_string(bit) + "]: " + bit_str + " (0: " + std::to_string(zero_count) + ", 1: " + std::to_string(one_count) + ")");
#endif

        // current_data を再構築 (0 バケット -> 1 バケット の順)
        // insert() + end() でループするより、コピー関数を使う方が高速なことが多い
        {
            // zero_bucket の先頭 zero_count 個をコピー
            // one_bucket の先頭 one_count 個をコピー
            // 先に resize(length_) しているので clear() は不要
            size_t pos = 0;
            // 0 バケットの要素コピー
            std::copy(zero_bucket.begin(), zero_bucket.begin() + zero_count, current_data.begin() + pos);
            pos += zero_count;
            // 1 バケットの要素コピー
            std::copy(one_bucket.begin(), one_bucket.begin() + one_count, current_data.begin() + pos);
        }
    }

    SetRank1Tables();
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    PrintRank0Tables();
    PrintRank1Tables();
    Logger::DebugLog(LOC, "WaveletMatrix Build - Done");
#endif
}

void WaveletMatrix::SetRank1Tables() {
    rank1_tables_.resize(sigma_);
    for (size_t b = 0; b < sigma_; ++b) {
        rank1_tables_[b].resize(rank0_tables_[b].size());
        size_t offset = rank0_tables_[b].back();
        for (size_t i = 0; i < rank0_tables_[b].size(); ++i) {
            rank1_tables_[b][i] = i - rank0_tables_[b][i] + offset;
        }
    }
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
    Logger::DebugLog(LOC, "Alphabet size   : " + std::to_string(wm_.GetSigma()));
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

const std::vector<std::vector<uint32_t>> &FMIndex::GetRank0Tables() const {
    return wm_.GetRank0Tables();
}

const std::vector<std::vector<uint32_t>> &FMIndex::GetRank1Tables() const {
    return wm_.GetRank1Tables();
}

std::vector<std::vector<uint32_t>> FMIndex::ConvertToBitMatrix(const std::string &query) const {
    // 1) string -> ID
    std::vector<uint32_t> nums = wm_.GetMapper().ToIds(query);

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query: " + query);
    Logger::DebugLog(LOC, "Query as numbers: " + ToString(nums));
#endif

    // 2) Convert to bits
    std::vector<std::vector<uint32_t>> bits(nums.size(), std::vector<uint32_t>(wm_.GetSigma()));
    for (size_t i = 0; i < nums.size(); ++i) {
        uint32_t val = nums[i];
        for (size_t b = 0; b < wm_.GetSigma(); ++b) {
            bits[i][b] = (val >> b) & 1U;
        }
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (size_t i = 0; i < bits.size(); ++i) {
        Logger::DebugLog(LOC, "bit_row[" + std::to_string(i) + "]: " + ToString(bits[i]));
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

void FMIndex::BackwardSearch(char c, uint32_t &left, uint32_t &right) const {
    if (!wm_.GetMapper().IsValidChar(c)) {
        Logger::ErrorLog(LOC, "Invalid character '" + std::string(1, c) + "' in BackwardSearch");
    }
    uint32_t cid = wm_.GetMapper().ToId(c);
    left         = wm_.RankCF(cid, left);
    right        = wm_.RankCF(cid, right);
}

uint32_t FMIndex::ComputeLPMfromWM(const std::string &query) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "lpm_len(" + query + ")");
#endif

    // Backward search for last character
    uint32_t              left  = 0;
    uint32_t              right = static_cast<uint32_t>(bwt_str_.size());
    std::vector<uint32_t> intervals;
    // Traverse query from end to front
    for (size_t i = 0; i < query.size(); ++i) {
        char c = query[i];
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "(char " + std::string(1, c) + ") (l, r) == (" + std::to_string(left) + ", " + std::to_string(right) + ")");
#endif
        BackwardSearch(c, left, right);
        intervals.push_back(right - left);
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "(l, r) == (" + std::to_string(left) + ", " + std::to_string(right) + ")");
    Logger::DebugLog(LOC, "Intervals: " + ToString(intervals));
#endif
    uint32_t lpm_len = 0;
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (intervals[i] == 0) {
            break;
        }
        lpm_len++;
    }
    return lpm_len;
}

uint32_t FMIndex::ComputeLPMfromBWT(const std::string &query) const {
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
    //         Logger::DebugLog(LOC, "  '" + std::string(1, c) + "' : " + std::to_string(offset));
    //     }
    // #endif

    // Step 3: backward search with rank() + offset
    int                   f       = 0;
    int                   g       = static_cast<int>(n);
    uint32_t              lpm_len = 0;
    std::vector<uint32_t> intervals;

    for (size_t qi = 0; qi < query.size(); ++qi) {
        char        c     = query[qi];
        std::string bwt_f = bwt.substr(0, f);
        std::string bwt_g = bwt.substr(0, g);

        int rank_f = static_cast<int>(std::count(bwt_f.begin(), bwt_f.end(), c));
        int rank_g = static_cast<int>(std::count(bwt_g.begin(), bwt_g.end(), c));
        int offset = F[c];

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "(char: " + std::string(1, c) + ") (l, r) == (" +
                                  std::to_string(f) + ", " + std::to_string(g) + ")");
        Logger::DebugLog(LOC, "(char: " + std::string(1, c) + ") l = offset(" + std::to_string(offset) +
                                  ") + rank(" + std::string(1, c) + ", " + std::to_string(f) + ")(" +
                                  std::to_string(rank_f) + ")");
        Logger::DebugLog(LOC, "(char: " + std::string(1, c) + ") r = offset(" + std::to_string(offset) +
                                  ") + rank(" + std::string(1, c) + ", " + std::to_string(g) + ")(" +
                                  std::to_string(rank_g) + ")");
#endif

        f = offset + rank_f;
        g = offset + rank_g;

        if (f < g) {
            lpm_len++;
        }
        intervals.push_back(g - f);
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "(l, r) == (" + std::to_string(f) + ", " + std::to_string(g) + ")");
    Logger::DebugLog(LOC, "Intervals: " + ToString(intervals));
    Logger::DebugLog(LOC, "LPM length (without WM): " + std::to_string(lpm_len));
#endif

    return lpm_len;
}

}    // namespace wm
}    // namespace fsswm
