#ifndef WM_FMINDEX_H_
#define WM_FMINDEX_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "wavelet_matrix.h"

namespace fsswm {
namespace wm {

class FMIndex {
public:
    // Constructor: build FM-index from text
    FMIndex(const std::string &text);

    // Search pattern in text, returns the count of occurrences
    size_t count(const std::string &pattern) const;

    // Retrieve occurrence positions (can be slower; naive approach)
    // Returns all starting indices of matches
    std::vector<size_t> locate(const std::string &pattern) const;

private:
    std::string                        text_;          /**< original text + sentinel */
    std::string                        bwt_str_;       /**< BWT of text */
    std::vector<size_t>                suffix_array_;  /**< Naive suffix array */
    WaveletMatrix                      wm_;            /**< Wavelet matrix built over bwt_str (as integer array) */
    std::vector<uint32_t>              c_table_;       /**< C array: cumulative frequencies of each character */
    std::unordered_map<char, uint32_t> char2id_;       /**< Mapping from character to integer */
    std::vector<char>                  id2char_;       /**< Mapping from integer to character */
    size_t                             alphabet_size_; /**< Size of the alphabet */

    // Build naive suffix array
    void build_suffix_array();

    // Build BWT from suffix array
    void build_bwt();

    // Build c_table (F array) from BWT
    void build_ctable();

    // Convert BWT string to an integer array for wavelet matrix
    std::vector<uint32_t> bwt_to_ints() const;

    // Map char to ID (0-indexed)
    // If not found, create new mapping
    uint32_t char_to_id(char c);

    // Map ID back to char
    char id_to_char(uint32_t id) const;

    // Backward search [top, bottom) range
    void backward_search(char c, size_t &left, size_t &right) const;
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_FMINDEX_H_
