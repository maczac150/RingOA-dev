#ifndef WM_PLAIN_WM_H_
#define WM_PLAIN_WM_H_

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace fsswm {
namespace wm {

enum class CharType
{
    DNA,
    PROTEIN,
};

/**
 * @brief WaveletMatrix class
 * @tparam BITS Number of bits in the integer array (e.g., 3 for DNA, 5 for protein)
 */
class WaveletMatrix {
public:
    WaveletMatrix() = default;
    explicit WaveletMatrix(const size_t bits, const std::vector<uint32_t> &data);

    size_t size() const;
    size_t bits() const;

    uint32_t RankCF(uint32_t c, size_t position) const;

    const std::vector<std::vector<uint32_t>> &GetRank0Tables() const;

    void PrintRank0Tables() const;

private:
    size_t bits_   = 0;
    size_t length_ = 0;
    // rank0_tables_[b] は (length_ + 1) 長さを持ち、
    // rank0_tables_[b][i] は [0..i) の要素中「b ビットが 0 の数」を記録
    std::vector<std::vector<uint32_t>> rank0_tables_;

    void Build(const std::vector<uint32_t> &data);
};

class FMIndex {
public:
    // コンストラクタ (textと文字種を受け取る)
    // 例: FMIndex<3> fmDNA("ACGTACGT", CharType::DNA);
    //     FMIndex<5> fmProtein("ARNDCQEGH...", CharType::PROTEIN);
    FMIndex(const size_t bits, const std::string &text, const CharType type = CharType::DNA);

    const std::vector<std::vector<uint32_t>> &GetRank0Tables() const;

    std::vector<uint32_t> ConvertToBitVector(const std::string &query) const;

    // Search query in text, returns the Longest Prefix Match Length
    uint32_t LongestPrefixMatchLength(const std::string &query) const;
    uint32_t ComputeLPMwithSDSL(const std::string &text, const std::string &query) const;

private:
    size_t                             bits_;          /**< Number of bits in the integer array */
    std::string                        text_;          /**< original text + sentinel */
    std::string                        bwt_str_;       /**< BWT of text */
    WaveletMatrix                      wm_;            /**< Wavelet matrix built over bwt_str (as integer array) */
    std::unordered_map<char, uint32_t> char2id_;       /**< Mapping from character to integer */
    std::vector<char>                  id2char_;       /**< Mapping from integer to character */
    size_t                             alphabet_size_; /**< Size of the alphabet */

    // Initialize character mappings
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

#endif    // WM_PLAIN_WM_H_
