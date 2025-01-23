#ifndef WM_FMINDEX_H_
#define WM_FMINDEX_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "wavelet_matrix.h"

namespace fsswm {
namespace wm {

enum class CharType
{
    DNA,
    PROTEIN,
};

class FMIndex {
public:
    // Constructor: build FM-index from text
    FMIndex(const std::string &text, const CharType type = CharType::DNA);

    // Search query in text, returns the count of occurrences
    uint32_t Count(const std::string &query) const;

    // Search query in text, returns the Longest Prefix Match Length
    uint32_t LongestPrefixMatchLength(const std::string &query) const;

private:
    std::string                        text_;          /**< original text + sentinel */
    std::string                        bwt_str_;       /**< BWT of text */
    WaveletMatrix                      wm_;            /**< Wavelet matrix built over bwt_str (as integer array) */
    std::unordered_map<char, uint32_t> char2id_;       /**< Mapping from character to integer */
    std::vector<char>                  id2char_;       /**< Mapping from integer to character */
    size_t                             alphabet_size_; /**< Size of the alphabet */

    // Initialize character mappings based on type
    void InitializeCharMap(CharType type);

    // Build BWT from suffix array
    void BuildBwt();

    // Convert BWT string to an integer array for wavelet matrix
    std::vector<uint32_t> BwtToInts() const;

    // Backward search [top, bottom) range
    void BackwardSearch(char c, uint32_t &left, uint32_t &right) const;
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_FMINDEX_H_
