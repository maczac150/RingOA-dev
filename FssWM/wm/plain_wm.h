#ifndef WM_PLAIN_WM_H_
#define WM_PLAIN_WM_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace fsswm {
namespace wm {

/**
 * @brief Character type enumeration
 * This enum is used to specify the type of characters in the text.
 * It can be either DNA or protein.
 */
enum class CharType
{
    DNA,
    PROTEIN,
};

/**
 * @brief CharMapper class for character mapping
 */
class CharMapper {
public:
    CharMapper(CharType type = CharType::DNA);
    void Initialize(CharType type);

    const std::unordered_map<char, uint32_t> &GetMap() const;
    size_t                                    GetSigma() const;
    CharType                                  GetType() const;
    bool                                      IsValidChar(char c) const;

    std::vector<uint32_t> ToIds(const std::string &s) const;
    uint32_t              ToId(char c) const;
    std::string           ToString(const std::vector<uint32_t> &v) const;
    std::string           MapToString() const;

private:
    std::unordered_map<char, uint32_t> char2id_;
    std::vector<char>                  id2char_;
    size_t                             sigma_;
    CharType                           type_;
};

/**
 * @brief WaveletMatrix class for plain computation.
 */
class WaveletMatrix {
public:
    WaveletMatrix()                                     = default;
    WaveletMatrix(const WaveletMatrix &)                = default;
    WaveletMatrix(WaveletMatrix &&) noexcept            = default;
    WaveletMatrix &operator=(const WaveletMatrix &)     = default;
    WaveletMatrix &operator=(WaveletMatrix &&) noexcept = default;

    explicit WaveletMatrix(const std::string &data, const CharType type = CharType::DNA);
    explicit WaveletMatrix(const std::vector<uint32_t> &data, const size_t sigma);

    size_t                                    GetLength() const;
    size_t                                    GetSigma() const;
    const CharMapper                         &GetMapper() const;
    std::string                               GetMapString() const;
    const std::vector<uint32_t>              &GetData() const;
    const std::vector<std::vector<uint32_t>> &GetRank0Tables() const;
    const std::vector<std::vector<uint32_t>> &GetRank1Tables() const;
    void                                      PrintRank0Tables() const;
    void                                      PrintRank1Tables() const;

    uint32_t RankCF(uint32_t c, size_t position) const;

private:
    size_t                             length_;
    size_t                             sigma_;
    CharMapper                         mapper_;
    std::vector<uint32_t>              data_;
    std::vector<std::vector<uint32_t>> rank0_tables_;
    std::vector<std::vector<uint32_t>> rank1_tables_;

    void Build(const std::vector<uint32_t> &data);
    void SetRank1Tables();
};

class FMIndex {
public:
    FMIndex(const std::string &text, const CharType type = CharType::DNA);
    FMIndex(const FMIndex &)                = default;
    FMIndex(FMIndex &&) noexcept            = default;
    FMIndex &operator=(const FMIndex &)     = default;
    FMIndex &operator=(FMIndex &&) noexcept = default;

    const WaveletMatrix                      &GetWaveletMatrix() const;
    const std::vector<std::vector<uint32_t>> &GetRank0Tables() const;
    const std::vector<std::vector<uint32_t>> &GetRank1Tables() const;

    std::vector<std::vector<uint32_t>> ConvertToBitMatrix(const std::string &query) const;

    // Search query in text, returns the Longest Prefix Match Length
    uint32_t ComputeLPMfromWM(const std::string &query) const;
    uint32_t ComputeLPMfromBWT(const std::string &query) const;

private:
    std::string   text_;    /**< original text + sentinel */
    std::string   bwt_str_; /**< BWT of text */
    WaveletMatrix wm_;      /**< Wavelet matrix built over bwt_str (as integer array) */

    // Build BWT from suffix array
    void BuildBwt();

    // Backward search [top, bottom) range
    void BackwardSearch(char c, uint32_t &left, uint32_t &right) const;
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_PLAIN_WM_H_
