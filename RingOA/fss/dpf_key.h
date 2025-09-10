#ifndef FSS_DPF_KEY_H_
#define FSS_DPF_KEY_H_

#include <memory>

#include "fss.h"

namespace ringoa {
namespace fss {
namespace dpf {

class DpfParameters {
public:
    DpfParameters() = delete;
    explicit DpfParameters(
        const uint64_t n, const uint64_t e,
        EvalType   eval_type   = kOptimizedEvalType,
        OutputType output_mode = OutputType::kShiftedAdditive);

    uint64_t GetInputBitsize() const {
        return input_bitsize_;
    }
    uint64_t GetOutputBitsize() const {
        return element_bitsize_;
    }
    bool GetEnableEarlyTermination() const {
        return enable_et_;
    }
    uint64_t GetTerminateBitsize() const {
        return terminate_bitsize_;
    }
    EvalType GetEvalType() const {
        return fde_type_;
    }
    OutputType GetOutputType() const {
        return output_mode_;
    }

    void ReconfigureParameters(
        const uint64_t n, const uint64_t e,
        EvalType   eval_type   = kOptimizedEvalType,
        OutputType output_mode = OutputType::kShiftedAdditive);

    std::string GetParametersInfo() const;

    void PrintParameters() const;

private:
    uint64_t   input_bitsize_;
    uint64_t   element_bitsize_;
    bool       enable_et_;
    uint64_t   terminate_bitsize_;
    EvalType   fde_type_;
    OutputType output_mode_;

    void Resolve_();
    void ValidateOrThrow_() const;
};

struct DpfKey {
    uint64_t                 party_id;
    block                    init_seed;
    uint64_t                 cw_length;
    std::unique_ptr<block[]> cw_seed;
    std::unique_ptr<bool[]>  cw_control_left;
    std::unique_ptr<bool[]>  cw_control_right;
    block                    output;

    DpfKey() = delete;
    explicit DpfKey(const uint64_t id, const DpfParameters &params);
    ~DpfKey() = default;

    DpfKey(const DpfKey &)                = delete;
    DpfKey &operator=(const DpfKey &)     = delete;
    DpfKey(DpfKey &&) noexcept            = default;
    DpfKey &operator=(DpfKey &&) noexcept = default;

    bool operator==(const DpfKey &rhs) const;
    bool operator!=(const DpfKey &rhs) const {
        return !(*this == rhs);
    }

    size_t GetSerializedSize() const {
        return serialized_size_;
    }
    size_t CalculateSerializedSize() const;

    // Appends a binary representation of this key to 'buffer'.
    void Serialize(std::vector<uint8_t> &buffer) const;
    // Replaces the current content with the key encoded in 'buffer'.
    void Deserialize(const std::vector<uint8_t> &buffer);

    void PrintKey(const bool detailed = false) const;

private:
    DpfParameters params_;
    size_t        serialized_size_;
};

}    // namespace dpf
}    // namespace fss
}    // namespace ringoa

#endif    // FSS_DPF_KEY_H_
