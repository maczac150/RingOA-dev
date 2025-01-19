#include "fmindex.h"

#include <algorithm>

namespace fsswm {
namespace wm {

FMIndex::FMIndex(const std::string &text) {
    // 1) Append sentinel '$'
    text_ = text + '$';

    // 2) Build suffix array
    build_suffix_array();

    // 3) Build BWT
    build_bwt();

    // 4) Build char <-> ID mappings
    //    We assume at most 'A, C, G, T, $' if DNA, etc.
    //    But let's do dynamic mapping in case we have more characters
    {
        // Collect all unique chars in bwt_str_
        for (char c : bwt_str_) {
            char_to_id(c);    // fill char2id_ / id2char_ if not exist
        }
        alphabet_size_ = char2id_.size();
    }

    // 5) Build c_table (F array)
    build_ctable();

    // 6) Build wavelet matrix from bwt_str_
    auto arr = bwt_to_ints();
    wm_      = WaveletMatrix(arr);
}

// O(n^2 log n) approach for demonstration
void FMIndex::build_suffix_array() {
    // Create array of indices [0..n-1]
    size_t n = text_.size();
    suffix_array_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        suffix_array_[i] = i;
    }
    // Sort suffix array
    std::sort(suffix_array_.begin(), suffix_array_.end(), [&](size_t a, size_t b) {
        // Compare text[a..] and text[b..]
        return text_.compare(a, std::string::npos, text_, b, std::string::npos) < 0;
    });
}

void FMIndex::build_bwt() {
    size_t n = text_.size();
    bwt_str_.resize(n);

    // BWT[i] = text[(suffix_array[i] - 1) mod n]
    // If suffix_array[i] == 0, then BWT[i] = text[n-1]
    for (size_t i = 0; i < n; ++i) {
        size_t sa_i = suffix_array_[i];
        if (sa_i == 0) {
            bwt_str_[i] = text_[n - 1];
        } else {
            bwt_str_[i] = text_[sa_i - 1];
        }
    }
}

// c_table[ch_id] = number of all characters < ch_id in the text
// used for backward search offset
void FMIndex::build_ctable() {
    // count frequency
    std::vector<uint32_t> freq(alphabet_size_, 0);
    for (char c : bwt_str_) {
        uint32_t id = char2id_[c];
        freq[id]++;
    }
    // prefix sum
    c_table_.resize(alphabet_size_);
    uint32_t sum = 0;
    for (size_t i = 0; i < alphabet_size_; ++i) {
        c_table_[i] = sum;
        sum += freq[i];
    }
}

uint32_t FMIndex::char_to_id(char c) {
    auto it = char2id_.find(c);
    if (it == char2id_.end()) {
        uint32_t id = char2id_.size();
        char2id_[c] = id;
        id2char_.push_back(c);
        return id;
    } else {
        return it->second;
    }
}

char FMIndex::id_to_char(uint32_t id) const {
    return id2char_[id];
}

std::vector<uint32_t> FMIndex::bwt_to_ints() const {
    // Covert each bwt_str_[i] to integer
    std::vector<uint32_t> arr(bwt_str_.size());
    for (size_t i = 0; i < bwt_str_.size(); ++i) {
        arr[i] = char2id_.at(bwt_str_[i]);
    }
    return arr;
}

void FMIndex::backward_search(char c, size_t &left, size_t &right) const {
    // Map char -> ID
    auto it = char2id_.find(c);
    if (it == char2id_.end()) {
        // Character doesn't exist in BWT => No matches
        left  = 0;
        right = 0;
        return;
    }
    uint32_t cid = it->second;

    // How many c in [0, left) => rank(cid, left)
    // How many c in [0, right) => rank(cid, right)
    uint32_t left_occ  = wm_.rank(cid, left);
    uint32_t right_occ = wm_.rank(cid, right);

    // new_left = C[c] + left_occ
    // new_right = C[c] + right_occ
    left  = c_table_[cid] + left_occ;
    right = c_table_[cid] + right_occ;
}

size_t FMIndex::count(const std::string &pattern) const {
    // Backward search for last character
    size_t left = 0, right = bwt_str_.size();
    // Traverse pattern from end to front
    for (int i = pattern.size() - 1; i >= 0; --i) {
        char c = pattern[i];
        backward_search(c, left, right);
        if (left == right) {
            return 0;
        }
    }
    return right - left;
}

std::vector<size_t> FMIndex::locate(const std::string &pattern) const {
    // 1) backward search to get the [left, right) range
    size_t left = 0, right = bwt_str_.size();
    for (int i = pattern.size() - 1; i >= 0; --i) {
        char c = pattern[i];
        backward_search(c, left, right);
        if (left == right) {
            return {};
        }
    }

    // 2) Retrieve suffix array positions
    //    We have suffix_array_[left .. right-1]
    //    naive approach: just collect them
    std::vector<size_t> result;
    for (size_t i = left; i < right; ++i) {
        result.push_back(suffix_array_[i]);
    }
    // optional: sort the result
    std::sort(result.begin(), result.end());
    return result;
}

}    // namespace wm
}    // namespace fsswm
