#include "fmindex.h"

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

FMIndex::FMIndex(const std::string &text, const CharType type) {
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
    wm_ = WaveletMatrix(bwt_ints);
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

uint32_t FMIndex::Count(const std::string &query) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "count(" + query + ")");
#endif

    // Backward search for last character
    uint32_t left = 0, right = bwt_str_.size();
    // Traverse query from end to front
    for (size_t i = 0; i < query.size(); ++i) {
        char c = query[i];
        BackwardSearch(c, left, right);
        if (left >= right) {
            return 0;
        }
    }
    return right - left;
}

uint32_t FMIndex::LongestPrefixMatchLength(const std::string &query) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "lpm_len(" + query + ")");
#endif

    // Backward search for last character
    uint32_t              left = 0, right = bwt_str_.size();
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

}    // namespace wm
}    // namespace fsswm
