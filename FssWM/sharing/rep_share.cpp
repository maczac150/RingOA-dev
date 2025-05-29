#include "rep_share.h"

namespace fsswm {
namespace sharing {

// Explicit instantiations for common types
template struct RepShare<uint32_t>;
template struct RepShare<uint64_t>;
template struct RepShare<block>;
template struct RepShareVec<uint32_t>;
template struct RepShareVec<uint64_t>;
template struct RepShareVec<block>;
template struct RepShareMat<uint32_t>;
template struct RepShareMat<uint64_t>;
template struct RepShareMat<block>;

}    // namespace sharing
}    // namespace fsswm
