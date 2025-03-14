#include "plain_wm.h"

#include <algorithm>
#include <sdsl/csa_wt.hpp>
#include <sdsl/suffix_arrays.hpp>

#include "FssWM/utils/logger.h"
#include "FssWM/utils/utils.h"

namespace {

std::string MapToString(std::unordered_map<char, uint32_t> &map) {
    std::string result;
    for (const auto &pair : map) {
        result += pair.first;
        result += ":";
        result += std::to_string(pair.second);
        result += " ";
    }
    return result;
}

}    // namespace

namespace fsswm {
namespace wm {

WaveletMatrix::WaveletMatrix(const size_t bits, const std::vector<uint32_t> &data)
    : bits_(bits) {
    Build(data);
}

size_t WaveletMatrix::size() const {
    return length_;
}

size_t WaveletMatrix::bits() const {
    return bits_;
}

const std::vector<std::vector<uint32_t>> &WaveletMatrix::GetRank0Tables() const {
    return rank0_tables_;
}

void WaveletMatrix::PrintRank0Tables() const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (size_t bit = 0; bit < bits_; ++bit) {
        Logger::DebugLog(LOC, "rank0_tables_[" + std::to_string(bit) + "]: " + ToString(rank0_tables_[bit]));
    }
#endif
}

uint32_t WaveletMatrix::RankCF(uint32_t c, size_t position) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "RankCF(" + std::to_string(c) + ", " + std::to_string(position) + ")");
#endif

    // Traverse from the LSB to the MSB
    for (size_t bit = 0; bit < bits_; ++bit) {
        // Which bit are we looking at?
        bool bit_val = (c >> bit) & 1U;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "bit: " + std::to_string(bit) + " (" + std::to_string(bit_val) + ")");
#endif
        // Number of 1 bits in [0, position) at bit-level = (BITS-1-bit)
        uint32_t zeros_count = rank0_tables_[bit][position];
        if (!bit_val) {
            // 0 側に行くので、そのまま zeros_count が次の position
            position = zeros_count;
        } else {
            // 1 側に行くので、全体の 0 数（rank0_tables_[bit][length_]）を考慮
            position = (position - zeros_count) + rank0_tables_[bit][length_];
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "RankCF_" + std::to_string(bit) + "[" + std::to_string(bit) + "]: " + std::to_string(position));
#endif
    }
    return position;
}

void WaveletMatrix::Build(const std::vector<uint32_t> &data) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "WaveletMatrix Build...");
    Logger::DebugLog(LOC, "Data: " + ToString(data));
#endif
    length_ = data.size();
    if (length_ == 0) {
        for (size_t b = 0; b < bits_; ++b) {
            rank0_tables_[b].clear();
        }
        return;
    }

    // rank0_tables_[b] のサイズを length_+1 に一括確保し、[0]～[length_] を使う
    rank0_tables_.resize(bits_);
    for (size_t b = 0; b < bits_; ++b) {
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
    for (size_t bit = 0; bit < bits_; ++bit) {
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
        Logger::DebugLog(LOC, "bit vector   [" + std::to_string(bit) + "]: " + bit_str);
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

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    PrintRank0Tables();
    Logger::DebugLog(LOC, "WaveletMatrix Build - Done");
#endif
}

FMIndex::FMIndex(const size_t bits, const std::string &text, const CharType type)
    : bits_(bits) {
    // 1) Set text
    text_ = text;
    std::reverse(text_.begin(), text_.end());

    // 2) Initialize character mappings
    InitializeCharMap(type);

    // 3) Build BWT from text
    BuildBwt();
    std::vector<uint32_t> bwt_ints = BwtToInts();

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, kDash);
    Logger::DebugLog(LOC, "Text               : " + text_);
    Logger::DebugLog(LOC, "BWT                : " + bwt_str_);
    Logger::DebugLog(LOC, "Alphabet size      : " + std::to_string(alphabet_size_));
    Logger::DebugLog(LOC, "Char to ID mapping : " + MapToString(char2id_));
    Logger::DebugLog(LOC, "BWT as integers    : " + ToString(bwt_ints));
    Logger::DebugLog(LOC, kDash);
#endif

    // 4) Build wavelet matrix from bwt_str_
    wm_ = WaveletMatrix(bits, bwt_ints);
}

const std::vector<std::vector<uint32_t>> &FMIndex::GetRank0Tables() const {
    return wm_.GetRank0Tables();
}

std::vector<uint32_t> FMIndex::ConvertToBitVector(const std::string &query) const {
    // 1) 文字列 -> 数値列
    std::vector<uint32_t> nums;
    nums.reserve(query.size());
    for (char c : query) {
        auto it = char2id_.find(c);
        if (it == char2id_.end()) {
            // 未知文字の場合は 0 or 適当なハンドリング
            nums.push_back(0);
        } else {
            nums.push_back(it->second);
        }
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query: " + query);
    Logger::DebugLog(LOC, "Query as numbers: " + ToString(nums));
#endif

    // 2) 数値列 -> ビット列 (LSB -> MSB)
    //    1文字分の値について BITS回ビットを取り出す
    std::vector<uint32_t> bits(nums.size() * bits_);
    size_t                idx = 0;
    for (auto val : nums) {
        for (size_t b = 0; b < bits_; ++b) {
            bits[idx++] = (val >> b) & 1U;
        }
    }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Query as bits: " + ToString(bits));
#endif
    return bits;
}

void FMIndex::InitializeCharMap(CharType type) {
    // Clear existing maps
    char2id_.clear();
    id2char_.clear();

    switch (type) {
        case CharType::DNA:
            alphabet_size_ = 5;
            char2id_       = {{'$', 0}, {'A', 1}, {'C', 2}, {'G', 3}, {'T', 4}};
            break;
        case CharType::PROTEIN:
            alphabet_size_ = 21;
            char2id_       = {
                {'$', 0}, {'A', 1}, {'R', 2}, {'N', 3}, {'D', 4}, {'C', 5}, {'Q', 6}, {'E', 7}, {'G', 8}, {'H', 9}, {'I', 10}, {'L', 11}, {'K', 12}, {'M', 13}, {'F', 14}, {'P', 15}, {'S', 16}, {'T', 17}, {'W', 18}, {'Y', 19}, {'V', 20}};
            break;
        default:
            Logger::FatalLog(LOC, "Invalid character type");
            exit(EXIT_FAILURE);
    }

    // Reverse map for id -> char
    id2char_.resize(char2id_.size());
    for (const auto &[ch, id] : char2id_) {
        id2char_[id] = ch;
    }
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

std::vector<uint32_t> FMIndex::BwtToInts() const {
    // Covert each bwt_str_[i] to integer
    std::vector<uint32_t> arr(bwt_str_.size());
    for (size_t i = 0; i < bwt_str_.size(); ++i) {
        arr[i] = char2id_.at(bwt_str_[i]);
    }
    return arr;
}

void FMIndex::BackwardSearch(char c, uint32_t &left, uint32_t &right) const {
    // Convert char to ID:
    auto it = char2id_.find(c);
    if (it == char2id_.end()) {
        Logger::ErrorLog(LOC, "Character '" + std::string(1, c) + "' not found in alphabet");
        return;
    }
    uint32_t cid = it->second;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "Backward search for '" + std::string(1, c) + "' (ID: " + std::to_string(cid) + ")");
    Logger::DebugLog(LOC, "(left, right) before RankCF: (" + std::to_string(left) + ", " + std::to_string(right) + ")");
#endif
    left  = wm_.RankCF(cid, left);
    right = wm_.RankCF(cid, right);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "(left, right) after RankCF: (" + std::to_string(left) + ", " + std::to_string(right) + ")");
#endif
}

uint32_t FMIndex::LongestPrefixMatchLength(const std::string &query) const {
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
        BackwardSearch(c, left, right);
        intervals.push_back(right - left);
    }
    Logger::DebugLog(LOC, "Intervals: " + ToString(intervals));
    uint32_t lpm_len = 0;
    for (size_t i = 0; i < intervals.size(); ++i) {
        if (intervals[i] == 0) {
            break;
        }
        lpm_len++;
    }
    return lpm_len;
}

uint32_t FMIndex::ComputeLPMwithSDSL(const std::string &text, const std::string &query) const {
    sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024> fm_sdsl;
    sdsl::construct_im(fm_sdsl, text, 1);

    typename sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024>::size_type l = 0, r = fm_sdsl.size() - 1;

    std::vector<uint32_t> intervals;
    uint32_t              lpm_len = 0;

    for (auto c : query) {
        typename sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024>::char_type c_sdsl = c;
        typename sdsl::csa_wt<sdsl::wt_huff<sdsl::rrr_vector<127>>, 512, 1024>::size_type nl = 0, nr = 0;

        sdsl::backward_search(fm_sdsl, l, r, c_sdsl, nl, nr);

        l = nl;
        r = nr;

        if (l <= r) {
            lpm_len++;
        } else {
            break;
        }
        intervals.push_back(r + 1 - l);
    }

    fsswm::Logger::DebugLog(LOC, "Intervals: " + fsswm::ToString(intervals));
    return lpm_len;
}

}    // namespace wm
}    // namespace fsswm
