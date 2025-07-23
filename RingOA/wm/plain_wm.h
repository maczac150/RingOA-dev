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

    const std::unordered_map<char, uint64_t> &GetMap() const;
    size_t                                    GetSigma() const;
    CharType                                  GetType() const;
    bool                                      IsValidChar(char c) const;

    std::vector<uint64_t> ToIds(const std::string &s) const;
    uint64_t              ToId(char c) const;
    std::string           ToString(const std::vector<uint64_t> &v) const;
    std::string           MapToString() const;

private:
    std::unordered_map<char, uint64_t> char2id_;
    std::vector<char>                  id2char_;
    size_t                             sigma_;
    CharType                           type_;
};

/**
 * @brief WaveletMatrix class for plain computation.
 */
class WaveletMatrix {
public:
    WaveletMatrix() = default;

    explicit WaveletMatrix(const std::string &data, const CharType type = CharType::DNA);
    explicit WaveletMatrix(const std::vector<uint64_t> &data, const size_t sigma);

    size_t                       GetLength() const;
    size_t                       GetSigma() const;
    const CharMapper            &GetMapper() const;
    std::string                  GetMapString() const;
    const std::vector<uint64_t> &GetData() const;
    const std::vector<uint64_t> &GetRank0Tables() const;
    const std::vector<uint64_t> &GetRank1Tables() const;
    void                         PrintRank0Tables() const;
    void                         PrintRank1Tables() const;

    uint64_t RankCF(uint64_t c, size_t position) const;
    uint64_t kthSmallest(size_t l, size_t r, size_t k) const;

private:
    size_t                length_;
    size_t                sigma_;
    CharMapper            mapper_;
    std::vector<uint64_t> data_;
    std::vector<uint64_t> rank0_tables_;
    std::vector<uint64_t> rank1_tables_;

    void Build(const std::vector<uint64_t> &data);
};

class FMIndex {
public:
    FMIndex(const std::string &text, const CharType type = CharType::DNA);
    FMIndex(const FMIndex &)                = default;
    FMIndex(FMIndex &&) noexcept            = default;
    FMIndex &operator=(const FMIndex &)     = default;
    FMIndex &operator=(FMIndex &&) noexcept = default;

    const WaveletMatrix         &GetWaveletMatrix() const;
    const std::vector<uint64_t> &GetRank0Tables() const;
    const std::vector<uint64_t> &GetRank1Tables() const;

    std::vector<uint64_t> ConvertToBitMatrix(const std::string &query) const;

    // Search query in text, returns the Longest Prefix Match Length
    uint64_t ComputeLPMfromWM(const std::string &query) const;
    uint64_t ComputeLPMfromBWT(const std::string &query) const;

private:
    std::string   text_;    /**< original text + sentinel */
    std::string   bwt_str_; /**< BWT of text */
    WaveletMatrix wm_;      /**< Wavelet matrix built over bwt_str (as integer array) */

    // Build BWT from suffix array
    void BuildBwt();

    // Backward search [top, bottom) range
    void BackwardSearch(char c, uint64_t &left, uint64_t &right) const;
};

}    // namespace wm
}    // namespace fsswm

#endif    // WM_PLAIN_WM_H_
