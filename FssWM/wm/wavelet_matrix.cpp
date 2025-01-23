#include "wavelet_matrix.h"

namespace fsswm {
namespace wm {

WaveletMatrix::WaveletMatrix(const std::vector<uint32_t> &data) {
    Build(data);
}

size_t WaveletMatrix::size() const {
    return length_;
}

uint32_t WaveletMatrix::RankCF(uint32_t c, size_t position) const {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "RankCF(" + std::to_string(c) + ", " + std::to_string(position) + ")");
#endif

    // Traverse from the LSB to the MSB
    for (int bit = 0; bit < (int)BITS; ++bit) {
        // Which bit are we looking at?
        bool bit_val = (c >> bit) & 1U;

        // Number of 1 bits in [0, position) at bit-level = (BITS-1-bit)
        uint32_t pos_zeros = rank0_tables_[bit][position];

        if (!bit_val) {
            position = pos_zeros;
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "RankCF_0[" + std::to_string(bit) + "]: " + std::to_string(position));
#endif
        } else {
            position = (position - pos_zeros) + rank0_tables_[bit][length_];
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
            Logger::DebugLog(LOC, "RankCF_1[" + std::to_string(bit) + "]: " + std::to_string(position));
#endif
        }
    }

    return position;
}

void WaveletMatrix::Build(const std::vector<uint32_t> &data) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    Logger::DebugLog(LOC, "WaveletMatrix Build...");
    Logger::DebugLog(LOC, "Data: " + ToString(data));
#endif
    length_ = data.size();

    // Allocate rank0_tables_ for each bit
    for (int bit = 0; bit < (int)BITS; ++bit) {
        // size = length_+1 (prefix sums: rank0_tables_[bit][0] = 0)
        rank0_tables_[bit].resize(length_ + 1, 0);
    }

    // We'll reorder the data at each bit level
    std::vector<uint32_t> current_data = data;
    std::vector<uint32_t> zero_bucket, one_bucket;
    zero_bucket.reserve(length_);
    one_bucket.reserve(length_);

    // Build from the LSB to the MSB
    for (int bit = 0; bit < (int)BITS; ++bit) {
        zero_bucket.clear();
        one_bucket.clear();

        uint32_t    run_zeros = 0;    // running count of 1-bits
        std::string bit_str   = "";

        for (size_t i = 0; i < length_; ++i) {
            bool bit_val = (current_data[i] >> bit) & 1U;
            bit_str += std::to_string(bit_val);
            if (bit_val) {
                one_bucket.push_back(current_data[i]);
            } else {
                zero_bucket.push_back(current_data[i]);
                run_zeros++;
            }

            // rank0_tables_[bit][i+1] = number of 1s in [0..i]
            rank0_tables_[bit][i + 1] = run_zeros;
        }
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
        Logger::DebugLog(LOC, "bit vector   [" + std::to_string(bit) + "]: " + bit_str);
#endif

        // reorder current_data for next bit
        current_data.clear();
        current_data.insert(current_data.end(), zero_bucket.begin(), zero_bucket.end());
        current_data.insert(current_data.end(), one_bucket.begin(), one_bucket.end());
    }

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (int bit = 0; bit < (int)BITS; ++bit) {
        Logger::DebugLog(LOC, "rank0_tables_[" + std::to_string(bit) + "]: " + ToString(rank0_tables_[bit]));
    }
    Logger::DebugLog(LOC, "WaveletMatrix Build - Done");
#endif
}

}    // namespace wm
}    // namespace fsswm
